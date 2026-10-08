#include "BenchReport.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <sstream>

namespace tp::sketch2d::bench {

	namespace {
		const char* const kStatistics[] = { "mean", "median", "p95", "p99", "max", "total" };

		std::string format(const char* pattern, double value) {
			char text[64];
			std::snprintf(text, sizeof(text), pattern, value);
			return text;
		}

		std::string ms(double value) { return format("%.3f", value); }

		std::string percent(double fraction) { return format("%+.1f%%", fraction * 100.0); }

		std::vector<std::string> split(const std::string& line, char separator) {
			std::vector<std::string> fields;
			std::string field;
			std::istringstream input(line);
			while (std::getline(input, field, separator))
				fields.push_back(field);
			return fields;
		}

		std::string joined(const std::vector<double>& values) {
			std::string text;
			for (double value : values)
				text += (text.empty() ? "" : ",") + format("%.6f", value);
			return text;
		}

		std::vector<double> numbers(const std::string& text) {
			std::vector<double> values;
			for (const std::string& field : split(text, ','))
				if (!field.empty()) values.push_back(std::stod(field));
			return values;
		}

		double normalTail(double z) { return 0.5 * std::erfc(z / std::sqrt(2.0)); }

		constexpr size_t kExactSampleLimit = 20;

		double exactMannWhitneyPValue(size_t first, size_t second, double u) {
			const size_t maximum = first * second;
			std::vector<std::vector<double>> previous(second + 1, std::vector<double>(maximum + 1, 0.0));
			for (size_t m = 0; m <= second; m++)
				previous[m][0] = 1;
			for (size_t n = 1; n <= first; n++) {
				std::vector<std::vector<double>> current(second + 1, std::vector<double>(maximum + 1, 0.0));
				current[0][0] = 1;
				for (size_t m = 1; m <= second; m++) {
					for (size_t value = 0; value <= n * m; value++) {
						current[m][value] = current[m - 1][value] + (value >= m ? previous[m][value - m] : 0.0);
					}
				}
				previous = std::move(current);
			}

			const std::vector<double>& counts = previous[second];
			double total = 0;
			double lower = 0;
			double upper = 0;
			for (size_t value = 0; value <= maximum; value++) {
				total += counts[value];
				if ((double) value <= u + 1e-9) lower += counts[value];
				if ((double) value >= u - 1e-9) upper += counts[value];
			}
			return std::min(1.0, 2.0 * std::min(lower, upper) / total);
		}

		std::string spread(const SampleSummary& summary) {
			if (summary.count < 2) return ms(summary.median);
			return ms(summary.median) + " ±" + format("%.0f", summary.variation * 100.0) + "%";
		}

		std::string cell(const ScenarioResult& result, const std::string& metric, const std::string& statistic) {
			const Metric* found = result.metric(metric);
			return found ? ms(statisticValue(found->statistics, statistic)) : std::string("-");
		}

		std::vector<std::string> groupsInOrder(const std::vector<ScenarioResult>& results) {
			std::vector<std::string> groups;
			for (const ScenarioResult& result : results) {
				if (std::find(groups.begin(), groups.end(), result.group) == groups.end()) groups.push_back(result.group);
			}
			return groups;
		}

		void appendSummaryTable(std::string& text, const std::vector<ScenarioResult>& results, const std::string& group) {
			text += "| scenario | frames | dabs | input lat p95 | frame wall p50 | frame wall p95 | frame wall max | "
							"frame cpu p95 | process | draw | canvas gpu p95 | window gpu p95 |\n";
			text += "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
			for (const ScenarioResult& result : results) {
				if (result.group != group) continue;
				text += "| " + result.name + " | " + std::to_string(result.frames) + " | " + std::to_string(result.dabs) +
								" | " + cell(result, "input.latency", "p95") + " | " + cell(result, "frame.wall", "median") + " | " +
								cell(result, "frame.wall", "p95") + " | " + cell(result, "frame.wall", "max") + " | " +
								cell(result, "frame.cpu", "p95") + " | " + cell(result, "frame.process", "p95") + " | " +
								cell(result, "frame.draw", "p95") + " | " + cell(result, "gpu.canvas", "p95") + " | " +
								cell(result, "gpu.window", "p95") + " |\n";
			}
			text += "\n";
		}

		void appendSectionBreakdown(std::string& text, const ScenarioResult& result) {
			std::vector<const Metric*> sections;
			for (const Metric& metric : result.sectionMetrics)
				sections.push_back(&metric);
			std::sort(sections.begin(), sections.end(), [](const Metric* left, const Metric* right) {
				return left->statistics.total > right->statistics.total;
			});

			text += "#### " + result.name + "\n\n";
			text += result.description + ". " + format("%.0f", result.inputMs) + " ms of input, " +
							std::to_string(result.inputEvents) + " events, " + std::to_string(result.tiles) + " tiles, wall " +
							format("%.0f", result.wallMs) + " ms, process CPU " + format("%.0f", result.processCpuMs) + " ms.\n\n";
			text += "| section | calls | total ms | mean/frame | p95/frame | max/frame |\n";
			text += "|---|---:|---:|---:|---:|---:|\n";
			for (const Metric* metric : sections) {
				const auto calls = result.counterTotals.find("calls " + metric->name);
				text += "| " + metric->name + " | " +
								format("%.0f", calls == result.counterTotals.end() ? 0.0 : calls->second) + " | " +
								ms(metric->statistics.total) + " | " + ms(metric->statistics.mean) + " | " +
								ms(metric->statistics.p95) + " | " + ms(metric->statistics.maximum) + " |\n";
			}
			for (const Metric& metric : result.gpuMetrics) {
				text += "| " + metric.name + " (gpu) | " + std::to_string(metric.statistics.samples) + " | " +
								ms(metric.statistics.total) + " | " + ms(metric.statistics.mean) + " | " + ms(metric.statistics.p95) +
								" | " + ms(metric.statistics.maximum) + " |\n";
			}

			std::string counters;
			for (const auto& [name, value] : result.counterTotals) {
				if (name.rfind("calls ", 0) == 0) continue;
				counters += (counters.empty() ? "" : ", ") + name + " " + format("%.0f", value);
			}
			if (!counters.empty()) text += "\nCounters: " + counters + ".\n";
			text += "\n";
		}
	}

	const Metric* ScenarioResult::metric(const std::string& name) const {
		for (const std::vector<Metric>* list : { &frameMetrics, &sectionMetrics, &gpuMetrics }) {
			for (const Metric& entry : *list) {
				if (entry.name == name) return &entry;
			}
		}
		return nullptr;
	}

	const std::vector<KeyMetric>& keyMetrics() {
		static const std::vector<KeyMetric> metrics = {
			{ "input.latency", "p95" }, { "frame.wall", "median" }, { "frame.wall", "p95" },  { "frame.cpu", "p95" },
			{ "frame.process", "p95" }, { "frame.draw", "p95" },    { "brush.dab", "total" }, { "gpu.canvas", "p95" },
		};
		return metrics;
	}

	double statisticValue(const ProfileStatistics& statistics, const std::string& statistic) {
		if (statistic == "mean") return statistics.mean;
		if (statistic == "median") return statistics.median;
		if (statistic == "p95") return statistics.p95;
		if (statistic == "p99") return statistics.p99;
		if (statistic == "max") return statistics.maximum;
		if (statistic == "total") return statistics.total;
		return 0;
	}

	const BaselineValue*
	Baseline::find(const std::string& scenario, const std::string& metric, const std::string& statistic) const {
		for (const BaselineValue& value : values) {
			if (value.scenario == scenario && value.metric == metric && value.statistic == statistic) return &value;
		}
		return nullptr;
	}

	std::optional<Baseline> Baseline::parse(const std::string& tsv) {
		std::istringstream input(tsv);
		std::string line;
		Baseline baseline;
		bool sawHeader = false;
		while (std::getline(input, line)) {
			if (line.empty()) continue;
			if (line[0] == '#') {
				const std::vector<std::string> fields = split(line.substr(1), '\t');
				if (fields.size() == 2 && fields[0] == "revision") baseline.revision = fields[1];
				if (fields.size() == 3 && fields[0] == "group") baseline.groups[fields[1]] = fields[2];
				continue;
			}
			const std::vector<std::string> fields = split(line, '\t');
			if (!sawHeader) {
				if (fields.size() < 4 || fields[0] != "scenario") return std::nullopt;
				sawHeader = true;
				continue;
			}
			if (fields.size() < 4) return std::nullopt;
			BaselineValue value{ fields[0], fields[1], fields[2], std::stod(fields[3]), {} };
			if (fields.size() >= 5) value.repeats = numbers(fields[4]);
			if (value.repeats.empty()) value.repeats.push_back(value.value);
			baseline.values.push_back(std::move(value));
		}
		if (!sawHeader) return std::nullopt;
		return baseline;
	}

	std::vector<Metric> metricsFromRecordedFrames(const std::string& tsv) {
		std::istringstream input(tsv);
		std::string line;
		if (!std::getline(input, line)) return {};
		const std::vector<std::string> columns = split(line, '\t');
		std::vector<std::vector<double>> values(columns.size());
		while (std::getline(input, line)) {
			const std::vector<std::string> fields = split(line, '\t');
			for (size_t index = 1; index < fields.size() && index < columns.size(); index++)
				values[index].push_back(std::stod(fields[index]));
		}

		std::vector<Metric> metrics;
		for (size_t index = 1; index < columns.size(); index++) {
			std::string name = columns[index];
			if (name == "wall_ms") name = "frame.wall";
			if (name == "cpu_ms") name = "frame.cpu";
			std::vector<double> series = values[index];
			if (name.rfind("gpu.", 0) == 0) std::erase_if(series, [](double value) { return value <= 0; });
			metrics.push_back({ name, "ms", summarize(series), {}, true });
		}
		return metrics;
	}

	std::vector<ScenarioResult> resultsFromBaseline(const Baseline& stored) {
		std::vector<ScenarioResult> results;
		auto resultFor = [&](const std::string& name) -> ScenarioResult& {
			for (ScenarioResult& result : results) {
				if (result.name == name) return result;
			}
			ScenarioResult result;
			result.name = name;
			const auto group = stored.groups.find(name);
			result.group = group == stored.groups.end() ? std::string("stored") : group->second;
			results.push_back(std::move(result));
			return results.back();
		};

		for (const BaselineValue& value : stored.values) {
			ScenarioResult& result = resultFor(value.scenario);
			if (value.statistic == "count") {
				result.counterTotals[value.metric] = value.value;
				continue;
			}
			auto metric = std::find_if(result.frameMetrics.begin(), result.frameMetrics.end(), [&](const Metric& entry) {
				return entry.name == value.metric;
			});
			if (metric == result.frameMetrics.end()) {
				result.frameMetrics.push_back({ value.metric, "ms", {}, {}, true });
				metric = result.frameMetrics.end() - 1;
			}
			metric->repeats[value.statistic] = value.repeats;
			if (value.statistic == "median") metric->statistics.median = value.value;
			if (value.statistic == "p95") metric->statistics.p95 = value.value;
			if (value.statistic == "mean") metric->statistics.mean = value.value;
			if (value.statistic == "p99") metric->statistics.p99 = value.value;
			if (value.statistic == "max") metric->statistics.maximum = value.value;
			if (value.statistic == "total") metric->statistics.total = value.value;
		}
		return results;
	}

	std::vector<double> Metric::repeatValues(const std::string& statistic) const {
		const auto found = repeats.find(statistic);
		if (found != repeats.end() && !found->second.empty()) return found->second;
		return { statisticValue(statistics, statistic) };
	}

	SampleSummary describeSamples(const std::vector<double>& samples) {
		SampleSummary summary;
		summary.count = samples.size();
		if (samples.empty()) return summary;
		const ProfileStatistics statistics = summarize(samples);
		summary.median = statistics.median;
		if (samples.size() < 2 || statistics.mean <= 0) return summary;

		double squares = 0;
		for (double value : samples)
			squares += (value - statistics.mean) * (value - statistics.mean);
		summary.variation = std::sqrt(squares / (double) (samples.size() - 1)) / statistics.mean;
		return summary;
	}

	double mannWhitneyPValue(const std::vector<double>& before, const std::vector<double>& after) {
		const size_t first = before.size();
		const size_t second = after.size();
		if (first < 2 || second < 2) return 1;

		std::vector<std::pair<double, int>> pooled;
		for (double value : before)
			pooled.push_back({ value, 0 });
		for (double value : after)
			pooled.push_back({ value, 1 });
		std::sort(pooled.begin(), pooled.end());

		double rankSumAfter = 0;
		double tieCorrection = 0;
		for (size_t index = 0; index < pooled.size();) {
			size_t end = index;
			while (end < pooled.size() && pooled[end].first == pooled[index].first)
				end++;
			const double rank = ((double) index + (double) end + 1.0) / 2.0;
			const double ties = (double) (end - index);
			tieCorrection += ties * ties * ties - ties;
			for (size_t member = index; member < end; member++) {
				if (pooled[member].second == 1) rankSumAfter += rank;
			}
			index = end;
		}

		const double n1 = (double) first;
		const double n2 = (double) second;
		const double total = n1 + n2;
		const double u = rankSumAfter - n2 * (n2 + 1.0) / 2.0;
		if (tieCorrection == 0 && first <= kExactSampleLimit && second <= kExactSampleLimit)
			return exactMannWhitneyPValue(first, second, u);
		const double mean = n1 * n2 / 2.0;
		const double variance = n1 * n2 / 12.0 * ((total + 1.0) - tieCorrection / (total * (total - 1.0)));
		if (variance <= 0) return 1;
		const double z = (std::fabs(u - mean) - 0.5) / std::sqrt(variance);
		return std::min(1.0, 2.0 * normalTail(std::max(0.0, z)));
	}

	std::vector<double> falseDiscoveryAdjusted(const std::vector<double>& pValues) {
		const size_t count = pValues.size();
		std::vector<size_t> order(count);
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(), [&](size_t left, size_t right) { return pValues[left] < pValues[right]; });

		std::vector<double> adjusted(count, 1.0);
		double running = 1.0;
		for (size_t rank = count; rank > 0; rank--) {
			const size_t index = order[rank - 1];
			running = std::min(running, pValues[index] * (double) count / (double) rank);
			adjusted[index] = std::min(1.0, running);
		}
		return adjusted;
	}

	const char* verdictName(Verdict verdict) {
		switch (verdict) {
			case Verdict::Faster: return "faster";
			case Verdict::Slower: return "slower";
			case Verdict::WithinNoise: return "within noise";
			case Verdict::Unmeasured: return "unmeasured";
		}
		return "";
	}

	std::string resultsTsv(const RunInfo& info, const std::vector<ScenarioResult>& results) {
		std::string text = "#revision\t" + info.revision + "\n";
		text += "#build\t" + info.buildType + "\n";
		text += "#device\t" + info.device + "\n";
		text += "#date\t" + info.date + "\n";
		text += "#pacing\t" + info.pacing + "\n";
		text += "#repeats\t" + std::to_string(info.repeats) + "\n";
		for (const ScenarioResult& result : results)
			text += "#group\t" + result.name + "\t" + result.group + "\n";
		text += "scenario\tmetric\tstatistic\tvalue\trepeats\n";
		for (const ScenarioResult& result : results) {
			for (const std::vector<Metric>* list : { &result.frameMetrics, &result.sectionMetrics, &result.gpuMetrics }) {
				for (const Metric& metric : *list) {
					for (const char* statistic : kStatistics) {
						text += result.name + "\t" + metric.name + "\t" + statistic + "\t" +
										format("%.6f", statisticValue(metric.statistics, statistic)) + "\t" +
										joined(metric.repeatValues(statistic)) + "\n";
					}
				}
			}
			for (const auto& [name, value] : result.counterTotals)
				text += result.name + "\t" + name + "\tcount\t" + format("%.0f", value) + "\t\n";
		}
		return text;
	}

	std::vector<Change>
	compare(const Baseline& baseline, const std::vector<ScenarioResult>& results, const ComparisonPolicy& policy) {
		std::vector<Change> changes;
		for (const ScenarioResult& result : results) {
			for (const KeyMetric& key : keyMetrics()) {
				const Metric* metric = result.metric(key.metric);
				const BaselineValue* before = baseline.find(result.name, key.metric, key.statistic);
				if (!metric || !before) continue;

				const std::vector<double> afterSamples = metric->repeatValues(key.statistic);
				Change change;
				change.scenario = result.name;
				change.group = result.group;
				change.metric = key.metric;
				change.statistic = key.statistic;
				change.before = describeSamples(before->repeats);
				change.after = describeSamples(afterSamples);
				change.pValue = mannWhitneyPValue(before->repeats, afterSamples);

				const bool sampled = change.before.count >= 2 && change.after.count >= 2;
				const double difference = change.after.median - change.before.median;
				const double noise = std::max(change.before.variation, change.after.variation) * change.before.median;
				const bool large = std::fabs(difference) > policy.absoluteFloorMs &&
													 std::fabs(change.change()) > policy.minimumEffect && std::fabs(difference) > 2.0 * noise;
				change.verdict = !large ? Verdict::WithinNoise : (sampled ? Verdict::Slower : Verdict::Unmeasured);
				if (change.verdict == Verdict::Slower && difference < 0) change.verdict = Verdict::Faster;
				changes.push_back(change);
			}
		}

		std::vector<double> pValues;
		for (const Change& change : changes)
			pValues.push_back(change.pValue);
		const std::vector<double> qValues = falseDiscoveryAdjusted(pValues);
		for (size_t index = 0; index < changes.size(); index++) {
			Change& change = changes[index];
			change.qValue = qValues[index];
			const bool decided = change.verdict == Verdict::Slower || change.verdict == Verdict::Faster;
			if (decided && change.pValue >= policy.significance) change.verdict = Verdict::WithinNoise;
		}
		return changes;
	}

	std::vector<GroupChange> summarizeChanges(const std::vector<Change>& changes) {
		std::vector<GroupChange> groups;
		for (const Change& change : changes) {
			auto found = std::find_if(groups.begin(), groups.end(), [&](const GroupChange& group) {
				return group.group == change.group && group.metric == change.metric + " " + change.statistic;
			});
			if (found == groups.end()) {
				groups.push_back({ change.group, change.metric + " " + change.statistic, 0, 0, 0, 0 });
				found = groups.end() - 1;
			}
			found->scenarios++;
			found->geometricMeanRatio += std::log(std::max(change.ratio(), 1e-9));
			if (change.verdict == Verdict::Slower) found->slower++;
			if (change.verdict == Verdict::Faster) found->faster++;
		}
		for (GroupChange& group : groups)
			group.geometricMeanRatio = std::exp(group.geometricMeanRatio / (double) std::max<size_t>(group.scenarios, 1));
		return groups;
	}

	size_t countVerdicts(const std::vector<Change>& changes, Verdict verdict) {
		return (size_t) std::count_if(changes.begin(), changes.end(), [verdict](const Change& change) {
			return change.verdict == verdict;
		});
	}

	std::string markdownReport(
		const RunInfo& info,
		const std::vector<ScenarioResult>& results,
		const std::optional<Baseline>& baseline,
		const ComparisonPolicy& policy
	) {
		std::string text = "# sketch2d benchmark\n\n";
		text += "| | |\n|---|---|\n";
		text += "| revision | " + info.revision + " |\n";
		text += "| build | " + info.buildType + " |\n";
		text += "| device | " + info.device + " |\n";
		text += "| date | " + info.date + " |\n";
		text += "| inputs | " + info.inputs + " |\n";
		text += "| pacing | " + info.pacing + " |\n";
		text += "| repeats | " + std::to_string(info.repeats) + " |\n";
		text += "| peak memory | " + format("%.0f", info.peakResidentMegabytes) + " MB |\n";
		text += "| baseline | " + (baseline ? baseline->revision : std::string("none")) + " |\n\n";

		text += "All times are milliseconds. Input latency is the time from an event's recorded timestamp to the end of "
						"the frame that consumed it. Frame wall/cpu cover one main-loop iteration; process and draw are its "
						"input-handling and painting halves. GPU columns are timestamp-query durations of the canvas render "
						"pass and the window frame. Values in the summary are pooled over every frame of every repeat.\n\n";

		if (baseline) {
			const std::vector<Change> changes = compare(*baseline, results, policy);
			text += "## Comparison with " + baseline->revision + "\n\n";
			text += "Each key metric is computed once per repeat; the median over repeats is compared and its spread is "
							"the coefficient of variation. A change is reported as faster or slower when the Mann-Whitney U test "
							"(exact for small tie-free samples) gives p < " +
							format("%.2f", policy.significance) + ", the medians differ by more than " +
							format("%.0f", policy.minimumEffect * 100.0) + "% and " + format("%.2f", policy.absoluteFloorMs) +
							" ms, and the difference exceeds twice the run-to-run spread. With a single repeat on either side "
							"no significance test is possible; large differences are then listed as unmeasured. Testing many metrics "
							"produces some flags by chance: the expected count of false flags is shown next to the verdicts, and q "
							"is the Benjamini-Hochberg false-discovery-adjusted p over the whole run (q < 0.05 survives the "
							"multiple comparisons).\n\n";
			text +=
				"Expected false flags at this significance: " + format("%.1f", policy.significance * (double) changes.size()) +
				" of " + std::to_string(changes.size()) + " compared metrics.\n\n";
			text += "| verdict | metrics |\n|---|---:|\n";
			for (Verdict verdict : { Verdict::Slower, Verdict::Faster, Verdict::WithinNoise, Verdict::Unmeasured })
				text +=
					"| " + std::string(verdictName(verdict)) + " | " + std::to_string(countVerdicts(changes, verdict)) + " |\n";
			text += "\n### By group\n\nGeometric mean of after/before median ratios over the group's scenarios.\n\n";
			text += "| group | metric | scenarios | mean ratio | slower | faster |\n|---|---|---:|---:|---:|---:|\n";
			for (const GroupChange& group : summarizeChanges(changes)) {
				text += "| " + group.group + " | " + group.metric + " | " + std::to_string(group.scenarios) + " | " +
								format("%.3f", group.geometricMeanRatio) + " | " + std::to_string(group.slower) + " | " +
								std::to_string(group.faster) + " |\n";
			}

			text += "\n### Significant changes\n\n";
			bool any = false;
			for (const Change& change : changes) {
				if (change.verdict != Verdict::Slower && change.verdict != Verdict::Faster) continue;
				if (!any)
					text += "| scenario | metric | before | after | change | p | q | verdict "
									"|\n|---|---|---:|---:|---:|---:|---:|---|\n";
				any = true;
				text += "| " + change.scenario + " | " + change.metric + " " + change.statistic + " | " +
								spread(change.before) + " | " + spread(change.after) + " | " + percent(change.change()) + " | " +
								format("%.3f", change.pValue) + " | " + format("%.3f", change.qValue) + " | " +
								verdictName(change.verdict) + " |\n";
			}
			if (!any) text += "None.\n";

			text += "\n<details><summary>Every key metric against the baseline</summary>\n\n";
			text +=
				"| scenario | metric | before | after | change | p | q | verdict |\n|---|---|---:|---:|---:|---:|---:|---|\n";
			for (const Change& change : changes) {
				text += "| " + change.scenario + " | " + change.metric + " " + change.statistic + " | " +
								spread(change.before) + " | " + spread(change.after) + " | " + percent(change.change()) + " | " +
								format("%.3f", change.pValue) + " | " + format("%.3f", change.qValue) + " | " +
								verdictName(change.verdict) + " |\n";
			}
			text += "\n</details>\n\n";
		}

		text += "## Summary\n\n";
		for (const std::string& group : groupsInOrder(results)) {
			text += "### " + group + "\n\n";
			appendSummaryTable(text, results, group);
		}

		bool anyLive = false;
		for (const ScenarioResult& result : results)
			anyLive = anyLive || !result.liveMetrics.empty();
		if (anyLive) {
			text += "## Live session versus replay\n\n";
			text += "Live is what the profiler measured while the session was recorded on the user's machine; replay is "
							"this run. Large differences mean the replay does not reproduce the experience (other machine, "
							"build or load), not that either number is wrong.\n\n";
			for (const ScenarioResult& result : results) {
				if (result.liveMetrics.empty()) continue;
				text += "#### " + result.name + "\n\n";
				text += "| metric | live median | live p95 | live max | replay median | replay p95 | replay max |\n";
				text += "|---|---:|---:|---:|---:|---:|---:|\n";
				for (const char* name : { "frame.wall",
																	"frame.cpu",
																	"frame.process",
																	"frame.draw",
																	"ui.dispatch",
																	"brush.dab",
																	"canvas.render",
																	"canvas.upload",
																	"gpu.canvas",
																	"gpu.window" }) {
					const Metric* replay = result.metric(name);
					const auto live =
						std::find_if(result.liveMetrics.begin(), result.liveMetrics.end(), [&](const Metric& metric) {
							return metric.name == name;
						});
					if (!replay || live == result.liveMetrics.end()) continue;
					text += "| " + std::string(name) + " | " + ms(live->statistics.median) + " | " + ms(live->statistics.p95) +
									" | " + ms(live->statistics.maximum) + " | " + ms(replay->statistics.median) + " | " +
									ms(replay->statistics.p95) + " | " + ms(replay->statistics.maximum) + " |\n";
				}
				text += "\n";
			}
		}

		text += "## Processing breakdown\n\n";
		text += "Section times are inclusive: frame.present contains frame.draw, which contains ui.paint, canvas.view and "
						"canvas.render (composite, mips, upload, record); frame.process contains ui.dispatch, which contains "
						"stroke.paint and brush.dab (brush.stamp or brush.smudge_dab). Calls and counters are per repeat.\n\n";
		for (const ScenarioResult& result : results)
			appendSectionBreakdown(text, result);
		return text;
	}
}
