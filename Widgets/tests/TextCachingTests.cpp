#include "BoxLayout.hpp"
#include "Label.hpp"
#include "RichText.hpp"
#include "TextArea.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <string>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	std::string longParagraph(int words) {
		std::string text;
		for (int index = 0; index < words; index++)
			text += (index ? " " : "") + std::string("word") + std::to_string(index);
		return text;
	}

	std::string manyLines(int lines) {
		std::string text;
		for (int index = 0; index < lines; index++)
			text += "line " + std::to_string(index) + "\n";
		return text;
	}

	struct ColumnScene {
		Widget root;

		explicit ColumnScene(Widget& child, halnf width) {
			root.emplaceLayout<ColumnLayout>(0);
			child.setSizing(AxisSizing::fixed(width), AxisSizing::fit());
			root.addChild(child);
		}
	};

	size_t measurementsDuringRepaint(UiDriver& driver) {
		driver.paint();
		const size_t before = driver.textMeasurements();
		driver.paint();
		driver.paint();
		return driver.textMeasurements() - before;
	}

	size_t textCommands(RecordingPainter& painter) {
		size_t count = 0;
		for (const PaintCommand& command : painter.commands())
			if (command.kind == PaintCommand::Kind::Text) count++;
		return count;
	}
}

SUITE(TextCaching) {
	TEST(WrappedLabelRepaintsWithoutMeasuring) {
		Label label(longParagraph(80));
		label.setWrap(true);
		ColumnScene scene(label, 150);
		UiDriver driver(scene.root);

		CHECK_EQUAL(0u, measurementsDuringRepaint(driver));
	}

	TEST(LabelRewrapsWhenTextOrWidthChanges) {
		Label label("one two three four");
		label.setWrap(true);
		const std::vector<std::string> narrow = label.linesFor(60);

		label.setText("one two three four five six");
		CHECK(label.linesFor(60).size() > narrow.size());
		CHECK_EQUAL(1u, label.linesFor(1000).size());
	}

	TEST(WrappingStopsAtTheLineLimit) {
		size_t calls = 0;
		const auto counting = [&calls](const std::string& line) {
			calls++;
			return (halnf) line.size() * 6;
		};

		CHECK_EQUAL(3u, wrapText(longParagraph(400), 120, counting, 2).size());
		const size_t limitedCalls = calls;

		calls = 0;
		CHECK(wrapText(longParagraph(400), 120, counting).size() > 50);
		CHECK(limitedCalls * 10 < calls);
	}

	TEST(LimitedLabelKeepsItsEllipsis) {
		Label label(longParagraph(400));
		label.setWrap(true);
		label.setMaximumLines(2);

		const std::vector<std::string>& lines = label.linesFor(120);
		REQUIRE CHECK_EQUAL(2u, lines.size());
		CHECK(lines[1].substr(lines[1].size() - 3) == "\u2026");
	}

	TEST(RichTextRepaintsWithoutMeasuring) {
		RichText text;
		text.setSpans({ { longParagraph(60) }, { " bold tail", FontFace::Bold } });
		ColumnScene scene(text, 180);
		UiDriver driver(scene.root);

		CHECK_EQUAL(0u, measurementsDuringRepaint(driver));
	}

	TEST(RichTextRelayoutAtNewWidthReusesWordWidths) {
		RichText text;
		text.setSpans({ { longParagraph(60) } });
		ColumnScene scene(text, 180);
		UiDriver driver(scene.root);
		driver.paint();

		const size_t before = driver.textMeasurements();
		const size_t lines = text.piecesFor(120).size();
		CHECK(lines > text.piecesFor(180).size());
		CHECK_EQUAL(before, driver.textMeasurements());
	}

	TEST(TextAreaRepaintsWithoutMeasuring) {
		TextArea area;
		area.setText(longParagraph(50) + "\n" + manyLines(20));
		ColumnScene scene(area, 200);
		UiDriver driver(scene.root);

		CHECK_EQUAL(0u, measurementsDuringRepaint(driver));
	}

	TEST(TextAreaPaintsOnlyVisibleLines) {
		TextArea area;
		area.setText(manyLines(400));
		ColumnScene scene(area, 200);
		UiDriver driver(scene.root, { 400, 300 });

		const size_t painted = textCommands(driver.paint());
		const size_t visible = size_t(300 / area.editor().lineHeight()) + 2;
		CHECK(painted <= visible);
		CHECK(painted > 0);
	}

	TEST(TextAreaWrapWidthTracksEdits) {
		TextArea area;
		area.setText("short");
		ColumnScene scene(area, 200);
		UiDriver driver(scene.root);
		CHECK_EQUAL(1u, area.editor().lines().size());

		area.setText(longParagraph(40));
		driver.layout();
		CHECK(area.editor().lines().size() > 1);
		for (const TextLayoutLine& line : area.editor().lines())
			CHECK(line.end >= line.start);
	}
}
