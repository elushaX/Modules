#pragma once

#include "Profiler.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace tp::sketch2d::bench {

	struct Metric {
		std::string name;
		std::string unit;
		ProfileStatistics statistics;
		std::map<std::string, std::vector<double>> repeats;
		bool lowerIsBetter = true;

		[[nodiscard]] std::vector<double> repeatValues(const std::string& statistic) const;
	};

	struct ScenarioResult {
		std::string name;
		std::string group;
		std::string input;
		std::string description;
		double inputMs = 0;
		double wallMs = 0;
		double processCpuMs = 0;
		size_t frames = 0;
		size_t inputEvents = 0;
		size_t dabs = 0;
		size_t tiles = 0;
		std::vector<Metric> frameMetrics;
		std::vector<Metric> sectionMetrics;
		std::vector<Metric> gpuMetrics;
		std::vector<Metric> liveMetrics;
		std::map<std::string, double> counterTotals;

		[[nodiscard]] const Metric* metric(const std::string& name) const;
	};

	struct RunInfo {
		std::string revision;
		std::string buildType;
		std::string device;
		std::string date;
		std::string inputs;
		std::string pacing;
		int repeats = 1;
		double peakResidentMegabytes = 0;
	};

	struct BaselineValue {
		std::string scenario;
		std::string metric;
		std::string statistic;
		double value = 0;
		std::vector<double> repeats;
	};

	struct Baseline {
		std::string revision;
		std::vector<BaselineValue> values;
		std::map<std::string, std::string> groups;

		[[nodiscard]] const BaselineValue*
		find(const std::string& scenario, const std::string& metric, const std::string& statistic) const;
		[[nodiscard]] static std::optional<Baseline> parse(const std::string& tsv);
	};

	struct SampleSummary {
		size_t count = 0;
		double median = 0;
		double variation = 0;
	};

	[[nodiscard]] SampleSummary describeSamples(const std::vector<double>& samples);
	[[nodiscard]] double mannWhitneyPValue(const std::vector<double>& before, const std::vector<double>& after);
	[[nodiscard]] std::vector<double> falseDiscoveryAdjusted(const std::vector<double>& pValues);

	enum class Verdict { Faster, Slower, WithinNoise, Unmeasured };

	[[nodiscard]] const char* verdictName(Verdict verdict);

	struct Change {
		std::string scenario;
		std::string group;
		std::string metric;
		std::string statistic;
		SampleSummary before;
		SampleSummary after;
		double pValue = 1;
		double qValue = 1;
		Verdict verdict = Verdict::Unmeasured;

		[[nodiscard]] double ratio() const { return before.median > 0 ? after.median / before.median : 1.0; }
		[[nodiscard]] double change() const { return ratio() - 1.0; }
	};

	struct ComparisonPolicy {
		double minimumEffect = 0.05;
		double absoluteFloorMs = 0.05;
		double significance = 0.01;
	};

	struct KeyMetric {
		const char* metric;
		const char* statistic;
	};

	struct GroupChange {
		std::string group;
		std::string metric;
		size_t scenarios = 0;
		double geometricMeanRatio = 1;
		size_t slower = 0;
		size_t faster = 0;
	};

	[[nodiscard]] const std::vector<KeyMetric>& keyMetrics();
	[[nodiscard]] double statisticValue(const ProfileStatistics& statistics, const std::string& statistic);

	[[nodiscard]] std::vector<Metric> metricsFromRecordedFrames(const std::string& tsv);
	[[nodiscard]] std::vector<ScenarioResult> resultsFromBaseline(const Baseline& stored);
	[[nodiscard]] std::string resultsTsv(const RunInfo& info, const std::vector<ScenarioResult>& results);
	[[nodiscard]] std::vector<Change>
	compare(const Baseline& baseline, const std::vector<ScenarioResult>& results, const ComparisonPolicy& policy);
	[[nodiscard]] std::vector<GroupChange> summarizeChanges(const std::vector<Change>& changes);
	[[nodiscard]] size_t countVerdicts(const std::vector<Change>& changes, Verdict verdict);
	[[nodiscard]] std::string markdownReport(
		const RunInfo& info,
		const std::vector<ScenarioResult>& results,
		const std::optional<Baseline>& baseline,
		const ComparisonPolicy& policy
	);
}
