#include "MarkdownEditor.hpp"
#include "TaskTreeView.hpp"
#include "WidgetApplication.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tp;
using namespace tp::taskboard;

namespace {
	constexpr int kWarmupFrames = 60;
	constexpr int kMeasuredFrames = 300;
	constexpr size_t kGraphTab = 0;
	constexpr size_t kKanbanTab = 1;
	constexpr int kTabSwitchPeriod = 20;
	constexpr halnf kNarrowColumn = 220;
	constexpr int kWidthSteps = 60;

	constexpr int kBacklogTasks = 44;
	constexpr int kOtherTasks = 14;

	std::string descriptionFor(int index) {
		const std::string sentence =
			"Investigate the widget behaviour on the board, compare it with the gallery and write the findings. ";
		std::string text;
		const int sentences = 1 + (index * 7) % 14;
		for (int part = 0; part < sentences; part++) {
			text += sentence;
			if (part % 4 == 3) text += "\n\n";
		}
		return text;
	}

	void populateRealisticBoard(TaskBoard& board) {
		for (int index = 0; index < kBacklogTasks + kOtherTasks; index++) {
			const TaskStatus status = index < kBacklogTasks ? TaskStatus::Backlog : kTaskStatuses[size_t(index) % 5];
			const std::string title = "Task number " + std::to_string(index) + " with a fairly long descriptive title";
			board.addTask({ title, descriptionFor(index), status, 1, "claude:opus" }, Vec2F(40.0f * (halnf) index, 0.0f));
		}
	}

	std::string markdownDocument() {
		std::string text;
		for (int section = 0; section < 30; section++) {
			text += "## Section " + std::to_string(section) + "\n\n";
			text += "Some **bold** text, some *italic* text and `inline code` with a [link](https://example.com). ";
			text += "This paragraph is long enough to wrap several times in a narrow preview pane of the editor.\n\n";
			text += "- first item\n- second item with more words\n- [ ] a task\n\n";
			text += "| Column | Value |\n|---|---|\n| a | 1 |\n| b | 2 |\n\n";
		}
		return text;
	}

	class MarkdownBenchApplication : public ui::WidgetApplication {
	public:
		MarkdownBenchApplication() {
			mEditor.setMode(ui::MarkdownEditor::Mode::Split);
			mEditor.setMarkdown(markdownDocument());
			setRoot(mEditor);
		}

		void forcePaint() { mEditor.requestPaint(); }
		[[nodiscard]] AppStorage persistentAppStorage() const override { return AppStorage::inHome("taskboard-bench"); }

		void setWidth(halnf width) { mEditor.setSizing(ui::AxisSizing::fixed(width), ui::AxisSizing::fill()); }

	private:
		ui::MarkdownEditor mEditor;
	};

	double millisecondsOf(time_ns nanoseconds) { return (double) nanoseconds / 1.0e6; }

	struct Series {
		std::vector<double> samples;

		void add(double value) { samples.push_back(value); }

		[[nodiscard]] double percentile(double fraction) const {
			std::vector<double> sorted = samples;
			std::sort(sorted.begin(), sorted.end());
			if (sorted.empty()) return 0;
			return sorted[std::min(sorted.size() - 1, (size_t) (fraction * (double) sorted.size()))];
		}
	};

	class TextBenchApplication : public ui::WidgetApplication {
	public:
		TextBenchApplication() {
			populateRealisticBoard(mBoard);
			setRoot(mView);
		}

		void showTab(size_t index) { mView.views().setCurrentIndex(index); }
		[[nodiscard]] AppStorage persistentAppStorage() const override { return AppStorage::inHome("taskboard-bench"); }

		void forcePaint() { mView.kanban().requestPaint(); }

		void forceLayout() { mView.kanban().requestLayout(); }

		void setColumnWidth(halnf width) { mView.kanban().setColumnWidthRange(width, width); }

	private:
		TaskBoard mBoard;
		TaskTreeView mView{ mBoard };
	};

	struct ScenarioResult {
		Series uiMs;
		Series frameMs;
		ualni paintedWidgets = 0;
	};

	template <typename Application, typename Prepare>
	ScenarioResult measure(Application& application, Prepare prepare) {
		ScenarioResult result;
		for (int frame = 0; frame < kWarmupFrames + kMeasuredFrames; frame++) {
			prepare(frame);
			application.ui().resetStats();
			const time_ns start = get_time_ns();
			application.step(16);
			const time_ns end = get_time_ns();
			if (frame < kWarmupFrames) continue;

			const ui::FrameStats& stats = application.ui().stats();
			result.uiMs.add(millisecondsOf(stats.layoutNs + stats.paintNs));
			result.frameMs.add(millisecondsOf(end - start));
			result.paintedWidgets = stats.paintedWidgets;
		}
		return result;
	}

	void report(const char* name, const ScenarioResult& result) {
		std::printf(
			"%-24s widgets %4lu | ui p50 %7.3f ms  p90 %7.3f ms | frame p50 %7.3f ms  p90 %7.3f ms\n",
			name,
			(unsigned long) result.paintedWidgets,
			result.uiMs.percentile(0.5),
			result.uiMs.percentile(0.9),
			result.frameMs.percentile(0.5),
			result.frameMs.percentile(0.9)
		);
	}

	void runKanban() {
		TextBenchApplication application;
		application.start();
		application.showTab(kKanbanTab);
		for (int frame = 0; frame < kWarmupFrames; frame++)
			application.step(16);

		report("kanban repaint", measure(application, [&](int) { application.forcePaint(); }));
		report("kanban relayout", measure(application, [&](int) { application.forceLayout(); }));
		report("kanban width animated", measure(application, [&](int frame) {
						 application.setColumnWidth(kNarrowColumn + (halnf) (frame % kWidthSteps));
					 }));
		application.setColumnWidth(kNarrowColumn);
		report("kanban tab switching", measure(application, [&](int frame) {
						 if (frame % kTabSwitchPeriod == 0)
							 application.showTab((frame / kTabSwitchPeriod) % 2 ? kGraphTab : kKanbanTab);
					 }));
		application.finish();
	}

	void reportPrefixWidthAgreement(const ui::TextMetrics& metrics) {
		const std::string sample = "AV To.WAY kerning, \u00e9t\u00e9 \u2014 emoji \U0001F600 tail";
		double worst = 0;
		for (ui::FontFace face : { ui::FontFace::Regular, ui::FontFace::Bold, ui::FontFace::Monospace }) {
			const ui::TextStyle style{ 13, face };
			const std::vector<halnf> prefixes = metrics.prefixWidths(sample, style);
			for (size_t length = 0; length <= sample.size(); length++) {
				const unsigned char next = length < sample.size() ? (unsigned char) sample[length] : 0;
				if ((next & 0xC0) == 0x80) continue;
				const halnf direct = metrics.styledTextWidth(sample.substr(0, length), style);
				worst = std::max(worst, (double) std::fabs(direct - prefixes[length]));
			}
		}
		std::printf("prefix width max deviation %.4f px\n", worst);
	}

	void runMarkdown() {
		MarkdownBenchApplication application;
		application.start();
		reportPrefixWidthAgreement(application.ui().textMetrics());
		for (int frame = 0; frame < kWarmupFrames; frame++)
			application.step(16);

		report("markdown repaint", measure(application, [&](int) { application.forcePaint(); }));
		report("markdown width animated", measure(application, [&](int frame) {
						 application.setWidth(900 + (halnf) (frame % kWidthSteps));
					 }));
		application.finish();
	}
}

int main(int argc, char** argv) {
	const bool markdown = argc > 1 && std::strcmp(argv[1], "markdown") == 0;
	if (markdown) runMarkdown();
	else runKanban();
	return 0;
}
