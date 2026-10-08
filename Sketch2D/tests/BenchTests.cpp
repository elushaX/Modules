#include "BenchInput.hpp"
#include "BenchReport.hpp"
#include "BenchScenario.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <set>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::sketch2d::bench;

namespace {
	Metric metricWithRepeats(const std::string& name, const std::string& statistic, const std::vector<double>& repeats) {
		Metric metric;
		metric.name = name;
		metric.unit = "ms";
		metric.statistics = summarize(repeats);
		metric.repeats[statistic] = repeats;
		return metric;
	}

	ScenarioResult resultWith(const std::string& name, const std::vector<double>& latencyP95, double dabTotal = 10) {
		ScenarioResult result;
		result.name = name;
		result.group = "brush size";
		result.description = "Round brush";
		result.frameMetrics.push_back(metricWithRepeats("input.latency", "p95", latencyP95));
		result.sectionMetrics.push_back(metricWithRepeats("brush.dab", "total", { dabTotal }));
		result.counterTotals["brush.dabs"] = 120;
		result.counterTotals["calls brush.dab"] = 120;
		return result;
	}

	RunInfo info() {
		RunInfo run;
		run.revision = "abc1234";
		run.buildType = "Release";
		return run;
	}

	const Change* changeOf(const std::vector<Change>& changes, const std::string& metric) {
		for (const Change& change : changes) {
			if (change.metric == metric) return &change;
		}
		return nullptr;
	}

	const std::vector<double> kSteady = { 4.0, 4.1, 3.9, 4.05, 3.95 };
}

SUITE(BenchInput) {
	TEST(ScribbleIsDeterministicFastAndInsideTheArea) {
		ScribbleOptions options;
		options.area = { 100, 50, 800, 600 };
		const InputRecording first = fastScribble(options);
		const InputRecording second = fastScribble(options);
		REQUIRE CHECK(first.events.size() > 500);
		CHECK(first.events == second.events);

		size_t presses = 0;
		size_t releases = 0;
		Vec2F previous = { -1, -1 };
		double travelled = 0;
		for (const RecordedInput& event : first.events) {
			if (event.kind == RecordedInput::Kind::Button) (event.flag ? presses : releases)++;
			if (event.kind != RecordedInput::Kind::Move) continue;
			CHECK(event.vector.x >= 100 && event.vector.x <= 900);
			CHECK(event.vector.y >= 50 && event.vector.y <= 650);
			if (previous.x >= 0) travelled += (event.vector - previous).length();
			previous = event.vector;
		}
		CHECK(presses >= 3);
		CHECK_EQUAL(presses, releases);
		CHECK(travelled / (first.durationMs() / 1000.0) > 1500);
	}

	TEST(ScribbleUsesVaryingPenPressure) {
		std::set<int> pressures;
		for (const RecordedInput& event : fastScribble({}).events) {
			if (event.kind == RecordedInput::Kind::Sample && event.sample.isStylus())
				pressures.insert((int) (event.sample.pressure * 10));
		}
		CHECK(pressures.size() >= 5);
	}

	TEST(ScribbleSurvivesSerialization) {
		const InputRecording recording = fastScribble({});
		const std::optional<InputRecording> parsed = InputRecording::parse(recording.serialize());
		REQUIRE CHECK(parsed.has_value());
		CHECK_EQUAL(recording.events.size(), parsed->events.size());
	}
}

SUITE(BenchScenarios) {
	TEST(BrushAndEraserMatrixCoversEveryAxis) {
		const std::vector<Scenario> scenarios = brushAndEraserScenarios({ "scribble" });
		std::set<std::string> names;
		std::set<std::string> groups;
		for (const Scenario& scenario : scenarios) {
			names.insert(scenario.name);
			groups.insert(scenario.group);
		}
		CHECK_EQUAL(scenarios.size(), names.size());
		const std::set<std::string> expected = { "brush size",       "zoom",       "brush preset",
																						 "brush parameters", "stabilizer", "eraser" };
		CHECK(expected == groups);
		CHECK(std::any_of(scenarios.begin(), scenarios.end(), [](const Scenario& scenario) {
			return scenario.brush == BrushType::Eraser && scenario.prepaint;
		}));
		CHECK(std::any_of(scenarios.begin(), scenarios.end(), [](const Scenario& scenario) {
			return scenario.eraserMode;
		}));
	}

	TEST(SeveralInputsPrefixScenarioNames) {
		const std::vector<Scenario> scenarios = brushAndEraserScenarios({ "scribble", "ilya" });
		CHECK(scenarios.front().name.rfind("scribble / ", 0) == 0);
		CHECK(scenarios.back().name.rfind("ilya / ", 0) == 0);
		CHECK_EQUAL("ilya", scenarios.back().input);
	}
}

SUITE(BenchStatistics) {
	TEST(SampleSummaryGivesMedianAndRelativeSpread) {
		const SampleSummary summary = describeSamples({ 9, 10, 11 });
		CHECK_EQUAL(3u, summary.count);
		CHECK_CLOSE(10.0, summary.median, 1e-9);
		CHECK_CLOSE(0.1, summary.variation, 1e-9);
		CHECK_EQUAL(0.0, describeSamples({ 5 }).variation);
	}

	TEST(MannWhitneySeparatesShiftedSamplesFromOverlappingOnes) {
		CHECK_CLOSE(2.0 / 252.0, mannWhitneyPValue({ 1, 2, 3, 4, 5 }, { 11, 12, 13, 14, 15 }), 1e-12);
		CHECK_CLOSE(4.0 / 252.0, mannWhitneyPValue({ 1, 2, 3, 4, 6 }, { 5, 12, 13, 14, 15 }), 1e-12);
		CHECK(mannWhitneyPValue({ 1, 3, 5, 7, 9 }, { 2, 4, 6, 8, 10 }) > 0.5);
		CHECK_EQUAL(1.0, mannWhitneyPValue({ 1 }, { 9 }));
		CHECK_CLOSE(mannWhitneyPValue({ 1, 2, 3 }, { 7, 8, 9 }), mannWhitneyPValue({ 7, 8, 9 }, { 1, 2, 3 }), 1e-12);
		CHECK(mannWhitneyPValue({ 5, 5, 5, 5 }, { 5, 5, 5, 5 }) > 0.9);
		CHECK(mannWhitneyPValue({ 1, 1, 1, 2, 2 }, { 2, 2, 3, 3, 3 }) < 0.1);
	}
}

SUITE(BenchMultipleComparisons) {
	TEST(BenjaminiHochbergMatchesTheTextbookExample) {
		const std::vector<double> adjusted = falseDiscoveryAdjusted({ 0.01, 0.04, 0.03, 0.005 });
		CHECK_CLOSE(0.02, adjusted[0], 1e-12);
		CHECK_CLOSE(0.04, adjusted[1], 1e-12);
		CHECK_CLOSE(0.04, adjusted[2], 1e-12);
		CHECK_CLOSE(0.02, adjusted[3], 1e-12);
	}

	TEST(ManyNoiseComparisonsFlagFewAndARealShiftIsFound) {
		uint32_t state = 12345;
		auto noise = [&state]() {
			state = state * 1664525u + 1013904223u;
			return 4.0 + ((double) (state >> 8) / 16777216.0 - 0.5) * 0.8;
		};
		auto sample = [&]() {
			std::vector<double> values;
			for (int repeat = 0; repeat < 5; repeat++)
				values.push_back(noise());
			return values;
		};

		std::vector<ScenarioResult> before;
		std::vector<ScenarioResult> after;
		for (int scenario = 0; scenario < 200; scenario++) {
			const std::string name = "case " + std::to_string(scenario);
			before.push_back(resultWith(name, sample()));
			after.push_back(resultWith(name, sample()));
		}
		std::vector<double> shifted;
		for (double value : before.front().frameMetrics.front().repeats["p95"])
			shifted.push_back(value * 2.0);
		after.front() = resultWith("case 0", shifted);

		const std::vector<Change> changes = compare(*Baseline::parse(resultsTsv(info(), before)), after, {});
		REQUIRE CHECK_EQUAL(200u, changes.size() / 2);
		CHECK(changeOf(changes, "input.latency")->verdict == Verdict::Slower);
		const size_t falseFlags = countVerdicts(changes, Verdict::Slower) + countVerdicts(changes, Verdict::Faster) - 1;
		CHECK(falseFlags <= 6);
	}
}

SUITE(BenchReport) {
	TEST(ResultsTsvRoundTripsWithRepeatSamples) {
		const std::string tsv = resultsTsv(info(), { resultWith("round size 20", kSteady, 40) });
		const std::optional<Baseline> baseline = Baseline::parse(tsv);
		REQUIRE CHECK(baseline.has_value());
		CHECK_EQUAL("abc1234", baseline->revision);
		const BaselineValue* latency = baseline->find("round size 20", "input.latency", "p95");
		REQUIRE CHECK(latency != nullptr);
		CHECK(latency->repeats == kSteady);
		CHECK_CLOSE(120.0, baseline->find("round size 20", "brush.dabs", "count")->value, 1e-6);
		CHECK(baseline->find("round size 20", "input.latency", "nonsense") == nullptr);
	}

	TEST(OlderBaselinesWithoutRepeatsStillParse) {
		const std::string tsv = "#revision\told\nscenario\tmetric\tstatistic\tvalue\nround\tinput.latency\tp95\t4.5\n";
		const std::optional<Baseline> baseline = Baseline::parse(tsv);
		REQUIRE CHECK(baseline.has_value());
		CHECK_EQUAL(1u, baseline->find("round", "input.latency", "p95")->repeats.size());
	}

	TEST(BaselineRejectsOtherFiles) { CHECK(!Baseline::parse("just text\n").has_value()); }

	TEST(NoisyButUnchangedRunsAreWithinNoise) {
		const Baseline baseline = *Baseline::parse(resultsTsv(info(), { resultWith("round", kSteady) }));
		const std::vector<Change> changes = compare(baseline, { resultWith("round", { 4.2, 3.8, 4.1, 4.0, 3.9 }) }, {});
		REQUIRE CHECK(changeOf(changes, "input.latency") != nullptr);
		CHECK(changeOf(changes, "input.latency")->verdict == Verdict::WithinNoise);
	}

	TEST(ConsistentShiftIsSlowerOrFasterWithItsPValue) {
		const Baseline baseline = *Baseline::parse(resultsTsv(info(), { resultWith("round", kSteady) }));

		const Change slower =
			*changeOf(compare(baseline, { resultWith("round", { 6.0, 6.1, 5.9, 6.05, 5.95 }) }, {}), "input.latency");
		CHECK(slower.verdict == Verdict::Slower);
		CHECK(slower.pValue < 0.05);
		CHECK_CLOSE(0.5, slower.change(), 1e-9);

		const Change faster =
			*changeOf(compare(baseline, { resultWith("round", { 2.0, 2.1, 1.9, 2.05, 1.95 }) }, {}), "input.latency");
		CHECK(faster.verdict == Verdict::Faster);
	}

	TEST(LargeButInconsistentShiftIsNotSignificant) {
		const Baseline baseline =
			*Baseline::parse(resultsTsv(info(), { resultWith("round", { 4.0, 4.0, 4.0, 4.0, 4.0 }) }));
		const std::vector<double> mixed = { 3.0, 3.5, 8.0, 8.5, 9.0 };
		ComparisonPolicy permissive;
		permissive.minimumEffect = 0;
		permissive.absoluteFloorMs = 0;
		const Change change = *changeOf(compare(baseline, { resultWith("round", mixed) }, permissive), "input.latency");
		CHECK(change.pValue > 0.05);
		CHECK(change.verdict == Verdict::WithinNoise);
	}

	TEST(ShiftSmallerThanTheRunToRunSpreadIsNoise) {
		const std::vector<double> wide = { 2.0, 3.0, 4.0, 5.0, 6.0, 2.5, 3.5, 4.5, 5.5, 6.5 };
		std::vector<double> shifted;
		for (double value : wide)
			shifted.push_back(value + 0.5);
		const Baseline baseline = *Baseline::parse(resultsTsv(info(), { resultWith("round", wide) }));
		ComparisonPolicy loose;
		loose.significance = 1.01;
		const Change change = *changeOf(compare(baseline, { resultWith("round", shifted) }, loose), "input.latency");
		CHECK(change.verdict == Verdict::WithinNoise);
	}

	TEST(SignificantButTinyShiftIsWithinNoise) {
		const Baseline baseline =
			*Baseline::parse(resultsTsv(info(), { resultWith("round", { 4.00, 4.01, 4.02, 4.03, 4.04 }) }));
		const std::vector<Change> changes =
			compare(baseline, { resultWith("round", { 4.10, 4.11, 4.12, 4.13, 4.14 }) }, {});
		CHECK(changeOf(changes, "input.latency")->verdict == Verdict::WithinNoise);
	}

	TEST(SingleRepeatCannotProveAChange) {
		const Baseline baseline = *Baseline::parse(resultsTsv(info(), { resultWith("round", { 4.0 }) }));
		CHECK(
			changeOf(compare(baseline, { resultWith("round", { 8.0 }) }, {}), "input.latency")->verdict == Verdict::Unmeasured
		);
		CHECK(
			changeOf(compare(baseline, { resultWith("round", { 4.05 }) }, {}), "input.latency")->verdict ==
			Verdict::WithinNoise
		);
	}

	TEST(GroupSummaryAveragesRatiosAcrossScenarios) {
		ScenarioResult small = resultWith("small", { 6.0, 6.1, 5.9, 6.05, 5.95 });
		ScenarioResult large = resultWith("large", kSteady);
		const Baseline baseline =
			*Baseline::parse(resultsTsv(info(), { resultWith("small", kSteady), resultWith("large", kSteady) }));
		const std::vector<GroupChange> groups = summarizeChanges(compare(baseline, { small, large }, {}));
		const auto latency = std::find_if(groups.begin(), groups.end(), [](const GroupChange& group) {
			return group.metric == "input.latency p95";
		});
		REQUIRE CHECK(latency != groups.end());
		CHECK_EQUAL(2u, latency->scenarios);
		CHECK_EQUAL(1u, latency->slower);
		CHECK_CLOSE(std::sqrt(1.5), latency->geometricMeanRatio, 1e-6);
	}

	TEST(MarkdownReportShowsVerdictCountsGroupsAndSpread) {
		const Baseline baseline = *Baseline::parse(resultsTsv(info(), { resultWith("round size 20", kSteady) }));
		const std::string report = markdownReport(
			info(), { resultWith("round size 20", { 6.0, 6.1, 5.9, 6.05, 5.95 }) }, baseline, ComparisonPolicy{}
		);
		CHECK(report.find("abc1234") != std::string::npos);
		CHECK(report.find("| slower | 1 |") != std::string::npos);
		CHECK(report.find("### By group") != std::string::npos);
		CHECK(report.find("| brush size | input.latency p95 | 1 | 1.500 | 1 | 0 |") != std::string::npos);
		CHECK(report.find("| round size 20 | input.latency p95 | 4.000 ±2% | 6.000 ±1% | +50.0% |") != std::string::npos);
		CHECK(report.find("#### round size 20") != std::string::npos);
		CHECK(report.find("| brush.dab | 120 |") != std::string::npos);
	}

	TEST(RecordedFramesBecomeLiveMetricsNextToTheReplay) {
		const std::string tsv = "frame\twall_ms\tcpu_ms\tbrush.dab\tgpu.canvas\n"
														"0\t2.0\t1.5\t0.5\t0.0\n"
														"1\t4.0\t3.5\t1.5\t0.2\n"
														"2\t6.0\t5.5\t2.5\t0.4\n";
		const std::vector<Metric> live = metricsFromRecordedFrames(tsv);
		REQUIRE CHECK_EQUAL(4u, live.size());
		CHECK_EQUAL("frame.wall", live[0].name);
		CHECK_CLOSE(4.0, live[0].statistics.median, 1e-9);
		CHECK_EQUAL("brush.dab", live[2].name);
		CHECK_EQUAL(2u, live[3].statistics.samples);

		ScenarioResult result = resultWith("session take", { 1.0 });
		result.group = "recorded sessions";
		result.frameMetrics.push_back(metricWithRepeats("frame.wall", "median", { 3.0, 3.0 }));
		result.liveMetrics = live;
		const std::string report = markdownReport(info(), { result }, std::nullopt, {});
		CHECK(report.find("## Live session versus replay") != std::string::npos);
		CHECK(report.find("| frame.wall | 4.000 |") != std::string::npos);
	}

	TEST(ReportWithoutBaselineSaysSo) {
		const std::string report = markdownReport(info(), { resultWith("round", { 1 }) }, std::nullopt, {});
		CHECK(report.find("| baseline | none |") != std::string::npos);
		CHECK(report.find("Comparison with") == std::string::npos);
	}
}
