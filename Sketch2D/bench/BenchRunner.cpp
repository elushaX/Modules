#include "BenchRunner.hpp"

#include "Sketch2DApplication.hpp"

#include <algorithm>

#include "Window.hpp"
#include <chrono>

namespace tp::sketch2d::bench {

	namespace {
		constexpr int kDrainFrameLimit = 400;
		constexpr halnf kPrepaintDabSize = 128;
		constexpr halnf kPrepaintStep = 48;
		constexpr double kFrameWaitSliceMs = 2;
		const char* const kRepeatStatistics[] = { "mean", "median", "p95", "p99", "max", "total" };

		double nowMs() {
			const auto now = std::chrono::steady_clock::now().time_since_epoch();
			return std::chrono::duration<double, std::milli>(now).count();
		}

		class MetricSeries {
		public:
			void add(const std::string& name, const std::vector<double>& values) {
				Entry& entry = entryFor(name);
				entry.pooled.insert(entry.pooled.end(), values.begin(), values.end());
				const ProfileStatistics repeat = summarize(values);
				for (const char* statistic : kRepeatStatistics)
					entry.repeats[statistic].push_back(statisticValue(repeat, statistic));
			}

			[[nodiscard]] std::vector<Metric> metrics() const {
				std::vector<Metric> result;
				for (const Entry& entry : mEntries) {
					Metric metric;
					metric.name = entry.name;
					metric.unit = "ms";
					metric.statistics = summarize(entry.pooled);
					metric.repeats = entry.repeats;
					result.push_back(std::move(metric));
				}
				return result;
			}

		private:
			struct Entry {
				std::string name;
				std::vector<double> pooled;
				std::map<std::string, std::vector<double>> repeats;
			};

			Entry& entryFor(const std::string& name) {
				for (Entry& entry : mEntries) {
					if (entry.name == name) return entry;
				}
				mEntries.push_back({ name, {}, {} });
				return mEntries.back();
			}

			std::vector<Entry> mEntries;
		};

		std::vector<double> frameValues(const std::vector<FrameProfile>& frames, double FrameProfile::* field) {
			std::vector<double> values;
			for (const FrameProfile& frame : frames)
				values.push_back(frame.*field);
			return values;
		}

		InputRecording fittedToWindow(const InputRecording& input, Vec2F windowSize) {
			if (input.windowSize.x <= 0 || input.windowSize.y <= 0) return input;
			if (input.windowSize == windowSize) return input;

			InputRecording fitted = input;
			const Vec2F scale = { windowSize.x / input.windowSize.x, windowSize.y / input.windowSize.y };
			for (RecordedInput& event : fitted.events) {
				if (event.kind != RecordedInput::Kind::Move) continue;
				event.vector = { event.vector.x * scale.x, event.vector.y * scale.y };
			}
			fitted.windowSize = windowSize;
			return fitted;
		}
	}

	void BenchRunner::apply(const Scenario& scenario) {
		Sketch2DApplication& application = mApplication;
		if (scenario.session) {
			scenario.session->applyTo(application.document(), application.brush(), application.root().canvas.settings());
			return;
		}
		Brush& target = application.brush();
		target.apply(*findBrushPreset(scenario.brush));
		target.setSize(scenario.size);
		target.setStabilizer(scenario.stabilizer);
		target.setEraserMode(scenario.eraserMode);
		if (scenario.tune) scenario.tune(target);

		ViewSettings& settings = application.root().canvas.settings();
		settings.useBrushStabilizer = true;

		application.document().resetView();
		application.document().setZoom(scenario.zoom);
	}

	BenchRunner::BenchRunner(Sketch2DApplication& application, RunOptions options) :
		mApplication(application),
		mOptions(options) {
		mApplication.window().setPlatformInputEnabled(false);
		mApplication.start();
		mApplication.window().injectInput().onKeyboardFocus(true);
		for (int frame = 0; frame < 3; frame++)
			mApplication.step(16);
	}

	BenchRunner::~BenchRunner() {
		mApplication.finish();
		mApplication.window().setPlatformInputEnabled(true);
	}

	void BenchRunner::resetDocument() {
		Profiler::global().setEnabled(false);
		mApplication.document().reset();
		mApplication.root().refreshFromState();
		for (int frame = 0; frame < 3; frame++)
			mApplication.step(16);
	}

	void BenchRunner::prepaint() {
		Document& document = mApplication.document();
		Brush paint;
		paint.apply(*findBrushPreset(BrushType::Round));
		paint.setSize(kPrepaintDabSize);
		paint.color = { 0.8f, 0.3f, 0.2f, 1.0f };

		const RectF canvas = mApplication.root().canvas.windowRect();
		const Vec2F topLeft = document.screenToWorld({ 0, 0 });
		const Vec2F bottomRight = document.screenToWorld(canvas.size);
		for (halnf y = topLeft.y; y <= bottomRight.y; y += kPrepaintStep / document.zoom()) {
			for (halnf x = topLeft.x; x <= bottomRight.x; x += kPrepaintStep / document.zoom())
				document.stampBrushDab({ x, y }, paint);
		}
		document.finishStroke("Prepaint");
		for (int frame = 0; frame < 3; frame++)
			mApplication.step(16);
	}

	void BenchRunner::replay(const InputRecording& input, std::vector<double>& latencies, size_t& frames) {
		WindowInputObserver& injector = mApplication.window().injectInput();
		Window& window = mApplication.window();
		const double start = nowMs();
		double previousFrame = start;
		double oldestPending = -1;
		size_t next = 0;

		while (next < input.events.size() || oldestPending >= 0) {
			const double elapsed = nowMs() - start;
			while (next < input.events.size() && input.events[next].timeMs <= elapsed) {
				if (oldestPending < 0) oldestPending = input.events[next].timeMs;
				input.events[next].deliverTo(injector);
				next++;
			}

			const double untilNext = next < input.events.size() ? input.events[next].timeMs - elapsed : 0.0;
			if (mOptions.pacedByCompositor && window.isWaitingForFrame()) {
				window.processEventsFor((halnf) std::clamp(untilNext, 0.05, kFrameWaitSliceMs));
				continue;
			}
			const bool wantsFrame = oldestPending >= 0 || mApplication.ui().needsFrame();
			if (!wantsFrame) {
				window.processEventsFor((halnf) std::clamp(untilNext, 0.05, kFrameWaitSliceMs));
				continue;
			}

			const double frameStart = nowMs();
			mApplication.step((halnf) (frameStart - previousFrame));
			previousFrame = frameStart;
			frames++;

			if (oldestPending >= 0) latencies.push_back(nowMs() - start - oldestPending);
			oldestPending = -1;
		}
	}

	void BenchRunner::drain(size_t& frames) {
		for (int frame = 0; frame < kDrainFrameLimit; frame++) {
			mApplication.step(16);
			frames++;
			if (!mApplication.ui().needsFrame() && !mApplication.root().canvas.isPainting()) break;
		}
	}

	ScenarioResult BenchRunner::run(const Scenario& scenario, const InputRecording& rawInput) {
		const InputRecording input = fittedToWindow(rawInput, mApplication.window().getSize());

		ScenarioResult result;
		result.name = scenario.name;
		result.group = scenario.group;
		result.input = scenario.input;
		result.description = scenario.describe();
		result.inputMs = input.durationMs();
		result.inputEvents = input.events.size();

		Profiler& profiler = Profiler::global();
		MetricSeries frameSeries;
		MetricSeries sectionSeries;
		MetricSeries gpuSeries;
		size_t frameCount = 0;
		const int repeats = std::max(1, mOptions.repeats);

		for (int repeat = 0; repeat < repeats; repeat++) {
			resetDocument();
			apply(scenario);
			mApplication.root().refreshFromState();
			if (scenario.prepaint) prepaint();
			mApplication.step(16);

			profiler.clear();
			profiler.setEnabled(true);
			const double cpuStart = Profiler::processCpuMs();
			const double wallStart = nowMs();

			std::vector<double> latencies;
			replay(input, latencies, frameCount);
			drain(frameCount);

			result.wallMs += (nowMs() - wallStart) / repeats;
			result.processCpuMs += (Profiler::processCpuMs() - cpuStart) / repeats;
			profiler.setEnabled(false);

			const std::vector<FrameProfile>& frames = profiler.frames();
			result.frames += frames.size();
			result.tiles = std::max<size_t>(result.tiles, mApplication.document().tileCount());

			frameSeries.add("frame.wall", frameValues(frames, &FrameProfile::wallMs));
			frameSeries.add("frame.cpu", frameValues(frames, &FrameProfile::cpuMs));
			frameSeries.add("input.latency", latencies);

			for (const std::string& name : profiler.counterNames())
				result.counterTotals[name] += profiler.counterTotal(name) / repeats;
			for (const std::string& name : profiler.sectionNames()) {
				result.counterTotals["calls " + name] += (double) profiler.sectionCallTotal(name) / repeats;
				sectionSeries.add(name, profiler.sectionSeries(name));
			}
			for (const std::string& name : profiler.gpuSectionNames()) {
				std::vector<double> measured;
				for (double value : profiler.gpuSeries(name)) {
					if (value > 0) measured.push_back(value);
				}
				gpuSeries.add(name, measured);
			}
		}

		result.frames /= (size_t) repeats;
		result.dabs = (size_t) result.counterTotals["brush.dabs"];
		result.frameMetrics = frameSeries.metrics();
		result.sectionMetrics = sectionSeries.metrics();
		result.gpuMetrics = gpuSeries.metrics();
		return result;
	}
}
