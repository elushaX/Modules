#include "ThreadPlacement.hpp"
#include "BenchInput.hpp"
#include "BenchReport.hpp"
#include "BenchRunner.hpp"
#include "Sketch2DApplication.hpp"
#include "VulkanContext.hpp"

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::sketch2d::bench;

namespace {
	struct Options {
		std::string outputDirectory;
		std::string baselinePath;
		std::string comparePath;
		std::vector<std::string> sessions;
		std::string revision = "unknown";
		std::string buildType = "unknown";
		std::string filter;
		std::vector<std::string> recordings;
		bool synthetic = true;
		bool quick = false;
		bool list = false;
		bool failOnRegression = false;
		bool unpaced = false;
		bool matrix = false;
		int repeats = 5;
		double minimumEffect = 0.05;
	};

	void printUsage() {
		std::printf(
			"Usage: sketch2dBench [options]\n"
			"\n"
			"Replays scripted drawing input through the real sketch2d application and writes a timing report.\n"
			"\n"
			"  --out DIR              directory for report.md and results.tsv (default bench-results)\n"
			"  --baseline FILE        results.tsv of an earlier run to compare against\n"
			"  --session DIR          replay a session recorded with Debug > Record session; repeatable.\n"
			"                         Only sessions run unless --matrix is also given; output defaults into DIR\n"
			"  --matrix               with --session, also run the brush and eraser matrix\n"
			"  --compare FILE         compare a stored results.tsv with --baseline without running\n"
			"  --recording FILE       add an input recording (input.txt of a session) as a scenario input; repeatable\n"
			"  --no-synthetic         do not use the generated fast scribble input\n"
			"  --filter TEXT          run only scenarios whose name contains TEXT\n"
			"  --repeats N            replay each scenario N times; statistics need at least 3 (default 5)\n"
			"  --quick                short synthetic input, for smoke runs\n"
			"  --min-effect FRACTION  smallest median change reported as faster or slower (default 0.05)\n"
			"  --fail-on-regression   exit with status 3 when a key metric is significantly slower\n"
			"  --unpaced              render frames back to back instead of waiting for the compositor\n"
			"  --revision TEXT        revision recorded in the report\n"
			"  --build TEXT           build type recorded in the report\n"
			"  --list                 print the scenario names and exit\n"
		);
	}

	std::optional<Options> parseOptions(int argc, char** argv) {
		Options options;
		for (int index = 1; index < argc; index++) {
			const std::string argument = argv[index];
			auto value = [&]() -> std::string { return index + 1 < argc ? argv[++index] : std::string(); };
			if (argument == "--out") options.outputDirectory = value();
			else if (argument == "--baseline") options.baselinePath = value();
			else if (argument == "--recording") options.recordings.push_back(value());
			else if (argument == "--compare") options.comparePath = value();
			else if (argument == "--session") options.sessions.push_back(value());
			else if (argument == "--no-synthetic") options.synthetic = false;
			else if (argument == "--filter") options.filter = value();
			else if (argument == "--repeats") options.repeats = std::atoi(value().c_str());
			else if (argument == "--quick") options.quick = true;
			else if (argument == "--min-effect") options.minimumEffect = std::atof(value().c_str());
			else if (argument == "--fail-on-regression") options.failOnRegression = true;
			else if (argument == "--unpaced") options.unpaced = true;
			else if (argument == "--matrix") options.matrix = true;
			else if (argument == "--revision") options.revision = value();
			else if (argument == "--build") options.buildType = value();
			else if (argument == "--list") options.list = true;
			else {
				printUsage();
				return std::nullopt;
			}
		}
		return options;
	}

	std::string readFile(const std::string& path) {
		std::ifstream file(path);
		std::stringstream text;
		text << file.rdbuf();
		return text.str();
	}

	bool writeFile(const std::filesystem::path& path, const std::string& text) {
		std::ofstream file(path);
		file << text;
		return (bool) file;
	}

	std::string today() {
		const std::time_t now = std::time(nullptr);
		char text[32];
		std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", std::localtime(&now));
		return text;
	}

	std::string deviceName(Window& window) {
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(window.vulkan()->physicalDevice(), &properties);
		return properties.deviceName;
	}

	std::vector<BenchInputs> loadInputs(const Options& options, RectF canvasArea) {
		std::vector<BenchInputs> inputs;
		if (options.synthetic) {
			ScribbleOptions scribble;
			scribble.area = canvasArea;
			if (options.quick) scribble.durationMs = 400;
			inputs.push_back({ "scribble", fastScribble(scribble) });
		}
		for (const std::string& path : options.recordings) {
			std::optional<InputRecording> recording = InputRecording::load(path);
			if (!recording) {
				std::fprintf(stderr, "sketch2dBench: %s is not an input recording\n", path.c_str());
				continue;
			}
			inputs.push_back({ std::filesystem::path(path).stem().string(), *recording });
		}
		return inputs;
	}

	const InputRecording* inputNamed(const std::vector<BenchInputs>& inputs, const std::string& name) {
		for (const BenchInputs& input : inputs) {
			if (input.name == name) return &input.recording;
		}
		return nullptr;
	}
}

int compareStored(const Options& options) {
	const std::optional<Baseline> before = Baseline::parse(readFile(options.baselinePath));
	const std::optional<Baseline> after = Baseline::parse(readFile(options.comparePath));
	if (!before || !after) {
		std::fprintf(stderr, "sketch2dBench: --compare needs two results.tsv files (--baseline and --compare)\n");
		return 2;
	}

	ComparisonPolicy policy;
	policy.minimumEffect = options.minimumEffect;
	RunInfo info;
	info.revision = after->revision;
	info.buildType = options.buildType;
	info.pacing = "stored";

	const std::vector<ScenarioResult> results = resultsFromBaseline(*after);
	const std::filesystem::path directory(options.outputDirectory);
	std::filesystem::create_directories(directory);
	writeFile(directory / "comparison.md", markdownReport(info, results, before, policy));

	const std::vector<Change> changes = compare(*before, results, policy);
	std::fprintf(
		stderr,
		"sketch2dBench: %zu key metrics compared: %zu slower, %zu faster, %zu within noise; report in %s\n",
		changes.size(),
		countVerdicts(changes, Verdict::Slower),
		countVerdicts(changes, Verdict::Faster),
		countVerdicts(changes, Verdict::WithinNoise),
		(directory / "comparison.md").c_str()
	);
	return options.failOnRegression && countVerdicts(changes, Verdict::Slower) > 0 ? 3 : 0;
}

namespace {
	struct ScratchStorage {
		std::filesystem::path root =
			std::filesystem::temp_directory_path() / ("sketch2d-bench-" + std::to_string(::getpid()));

		~ScratchStorage() {
			std::error_code error;
			std::filesystem::remove_all(root, error);
		}
	};
}

int main(int argc, char** argv) {
	const std::optional<Options> parsed = parseOptions(argc, argv);
	if (!parsed) return 2;
	const Options& options = *parsed;
	if (!options.comparePath.empty()) return compareStored(options);

	ThreadPlacement::reserveCallingThreadForInteraction();
	const ScratchStorage scratch;
	Sketch2DApplication application(Sketch2DOptions::forTesting(scratch.root.string()));
	BenchRunner runner(application, { options.repeats, !options.unpaced });

	std::vector<BenchInputs> inputs = loadInputs(options, application.root().canvas.windowRect());
	std::map<std::string, std::string> recordedFrames;
	if (inputs.empty() && options.sessions.empty()) {
		std::fprintf(stderr, "sketch2dBench: no inputs\n");
		return 2;
	}

	std::vector<std::string> inputNames;
	for (const BenchInputs& input : inputs)
		inputNames.push_back(input.name);

	std::vector<Scenario> scenarios;
	std::vector<BenchInputs> sessionInputs;
	for (const std::string& path : options.sessions) {
		std::optional<RecordedSession> session = RecordedSession::load(path);
		if (!session) {
			std::fprintf(stderr, "sketch2dBench: %s is not a recorded session\n", path.c_str());
			return 2;
		}
		Scenario scenario;
		scenario.name = "session " + std::filesystem::path(path).filename().string();
		scenario.group = "recorded sessions";
		scenario.input = scenario.name;
		scenario.session = std::make_shared<SessionSnapshot>(session->snapshot);
		scenarios.push_back(scenario);
		sessionInputs.push_back({ scenario.name, session->input });
		recordedFrames[scenario.name] = session->recordedFrames;
	}
	if (options.sessions.empty() || options.matrix) {
		for (const Scenario& scenario : brushAndEraserScenarios(inputNames))
			scenarios.push_back(scenario);
	}
	for (const BenchInputs& input : sessionInputs)
		inputs.push_back(input);

	std::vector<ScenarioResult> results;
	for (const Scenario& scenario : scenarios) {
		if (!options.filter.empty() && scenario.name.find(options.filter) == std::string::npos) continue;
		if (options.list) {
			std::printf("%s\n", scenario.name.c_str());
			continue;
		}
		std::fprintf(stderr, "sketch2dBench: %s\n", scenario.name.c_str());
		results.push_back(runner.run(scenario, *inputNamed(inputs, scenario.input)));
	}
	if (options.list) return 0;

	RunInfo info;
	info.revision = options.revision;
	info.buildType = options.buildType;
	info.device = deviceName(application.window());
	info.date = today();
	info.repeats = options.repeats;
	info.pacing = options.unpaced ? "unpaced (frames back to back)" : "compositor frame callbacks";
	info.peakResidentMegabytes = Profiler::peakResidentMegabytes();
	for (const std::string& name : inputNames)
		info.inputs += (info.inputs.empty() ? "" : ", ") + name;

	std::string outputDirectory = options.outputDirectory;
	if (outputDirectory.empty())
		outputDirectory = options.sessions.size() == 1 && !options.matrix
												? (std::filesystem::path(options.sessions.front()) / "replay").string()
												: std::string("bench-results");

	for (ScenarioResult& result : results) {
		const auto live = recordedFrames.find(result.name);
		if (live == recordedFrames.end() || live->second.empty()) continue;
		result.liveMetrics = metricsFromRecordedFrames(live->second);
	}

	std::optional<Baseline> baseline;
	if (!options.baselinePath.empty()) {
		baseline = Baseline::parse(readFile(options.baselinePath));
		if (!baseline)
			std::fprintf(stderr, "sketch2dBench: ignoring unreadable baseline %s\n", options.baselinePath.c_str());
	}

	ComparisonPolicy policy;
	policy.minimumEffect = options.minimumEffect;

	const std::filesystem::path directory(outputDirectory);
	std::filesystem::create_directories(directory);
	const bool written = writeFile(directory / "results.tsv", resultsTsv(info, results)) &&
											 writeFile(directory / "report.md", markdownReport(info, results, baseline, policy));
	if (!written) {
		std::fprintf(stderr, "sketch2dBench: could not write into %s\n", directory.c_str());
		return 1;
	}

	const std::vector<Change> changes = baseline ? compare(*baseline, results, policy) : std::vector<Change>{};
	const size_t regressions = countVerdicts(changes, Verdict::Slower);
	std::fprintf(
		stderr,
		"sketch2dBench: %zu scenarios, %zu key metrics compared: %zu slower, %zu faster, %zu within noise, %zu unmeasured; "
		"report in %s\n",
		results.size(),
		changes.size(),
		regressions,
		countVerdicts(changes, Verdict::Faster),
		countVerdicts(changes, Verdict::WithinNoise),
		countVerdicts(changes, Verdict::Unmeasured),
		(directory / "report.md").c_str()
	);
	return options.failOnRegression && regressions > 0 ? 3 : 0;
}
