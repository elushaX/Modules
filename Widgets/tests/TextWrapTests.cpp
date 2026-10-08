#include "BoxLayout.hpp"
#include "Dialog.hpp"
#include "Label.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"
#include "Views.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	halnf monospace(const std::string& text) { return MonospaceTextMetrics().textWidth(text, 10); }

	const Metrics& metrics() { return Theme::standard().metrics; }
}

SUITE(TextWrapping) {
	TEST(WordsWrapAtTheWidth) {
		const auto lines = wrapText("one two three four", monospace("three four") + 0.1f, monospace);
		REQUIRE CHECK_EQUAL((size_t) 2, lines.size());
		CHECK_EQUAL(std::string("one two"), lines[0]);
		CHECK_EQUAL(std::string("three four"), lines[1]);
	}

	TEST(NewlinesStartNewLinesAndEmptyLinesStay) {
		const auto lines = wrapText("first\n\nsecond line", 1000, monospace);
		REQUIRE CHECK_EQUAL((size_t) 3, lines.size());
		CHECK_EQUAL(std::string("first"), lines[0]);
		CHECK_EQUAL(std::string(""), lines[1]);
		CHECK_EQUAL(std::string("second line"), lines[2]);
	}

	TEST(LongWordsBreakInsideTheWord) {
		const auto lines = wrapText("abcdefghij", monospace("abcd") + 0.1f, monospace);
		REQUIRE CHECK_EQUAL((size_t) 3, lines.size());
		CHECK_EQUAL(std::string("abcd"), lines[0]);
		CHECK_EQUAL(std::string("efgh"), lines[1]);
		CHECK_EQUAL(std::string("ij"), lines[2]);
		for (const std::string& line : lines)
			CHECK(monospace(line) <= monospace("abcd") + 0.1f);
	}

	TEST(NothingIsLost) {
		const std::string text = "- Give the colour popup wheel, square, RGB, HSV and HSL pages";
		std::string joined;
		for (const std::string& line : wrapText(text, monospace("- Give the colour"), monospace))
			joined += (joined.empty() ? "" : " ") + line;
		CHECK_EQUAL(text, joined);
	}

	TEST(WrappingLabelGrowsTallerInsteadOfOverflowing) {
		Label label("- Give the colour popup wheel, square, RGB, HSV and HSL pages to the picker");
		label.setWrap(true);
		label.setSizing(AxisSizing::fill(), AxisSizing::fit());
		Widget root;
		root.emplaceLayout<ColumnLayout>(0);
		root.addChild(label);
		UiDriver driver(root, { 160, 400 });

		const RectF rect = label.windowRect();
		CHECK(rect.size.x <= 160.5f);
		const auto lines = label.linesFor(rect.size.x);
		CHECK(lines.size() > 1);
		CHECK_CLOSE(metrics().lineHeight * (halnf) lines.size(), rect.size.y, 0.5f);
		RecordingPainter& painter = driver.paint();
		size_t painted = 0;
		for (const PaintCommand& command : painter.commands())
			if (command.kind == PaintCommand::Kind::Text) {
				painted++;
				CHECK(
					command.rect.pos.x + MonospaceTextMetrics().textWidth(command.text, metrics().fontSize) <=
					rect.pos.x + rect.size.x + 0.5f
				);
			}
		CHECK_EQUAL(lines.size(), painted);
	}

	TEST(LabelsWithoutWrapKeepOneLine) {
		Label label("a long single line of text that does not wrap");
		Widget root;
		root.emplaceLayout<ColumnLayout>(0);
		root.addChild(label);
		UiDriver driver(root, { 100, 200 });
		CHECK_CLOSE(metrics().lineHeight, label.windowRect().size.y, 0.5f);
	}

	TEST(DialogMessagesWrap) {
		Dialog dialog("A title that is long enough to need more than one line in the dialog");
		dialog.setMessage(
			"This message is also long enough that it would be cut off by the dialog edge if it did not wrap at all"
		);
		Widget root;
		root.emplaceLayout<ColumnLayout>(0);
		root.addChild(dialog);
		UiDriver driver(root, { 800, 600 });
		const RectF box = dialog.windowRect();
		size_t texts = 0;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Text) {
				texts++;
				CHECK(MonospaceTextMetrics().textWidth(command.text, metrics().fontSize) <= command.rect.size.x + 0.5f);
				CHECK(command.rect.pos.x + command.rect.size.x <= box.pos.x + box.size.x + 0.5f);
			}
		CHECK(texts > 2);
	}
}

SUITE(TabScrolling) {
	struct FiveTabs {
		TabView tabs;
		Widget pages[5];
		Widget root;
		static constexpr const char* kTitles[] = { "Wheel", "Square", "RGB", "HSV", "HSL" };

		explicit FiveTabs(halnf width) {
			for (int4 index = 0; index < 5; index++)
				tabs.addTab(kTitles[index], pages[index]);
			tabs.setSizing(AxisSizing::fixed(width), AxisSizing::fixed(200));
			root.emplaceLayout<ColumnLayout>(0);
			root.addChild(tabs);
		}
	};

	TEST(TabsKeepTheirFullNamesAndScroll) {
		FiveTabs scene(150);
		UiDriver driver(scene.root, { 400, 400 });
		for (size_t index = 0; index < scene.tabs.tabCount(); index++) {
			const RectF tab = scene.tabs.tabButton(index).windowRect();
			CHECK(
				tab.size.x >= MonospaceTextMetrics().textWidth(FiveTabs::kTitles[index], metrics().fontSize) +
												metrics().controlPadding.total(Axis::Horizontal) - 0.5f
			);
		}
		CHECK(scene.tabs.tabScroller().isScrollable());
		CHECK_CLOSE(150.0f, scene.tabs.tabScroller().windowRect().size.x, 0.5f);
	}

	TEST(WheelScrollsTheTabBar) {
		FiveTabs scene(150);
		UiDriver driver(scene.root, { 400, 400 });
		const RectF bar = scene.tabs.tabScroller().windowRect();
		driver.moveTo(bar.center());
		driver.wheel(-3);
		driver.settle();
		CHECK(scene.tabs.tabScroller().offset() > 0);
		const RectF last = scene.tabs.tabButton(4).windowRect();
		CHECK(last.pos.x + last.size.x <= bar.pos.x + bar.size.x + 0.5f + 40);
	}

	TEST(TouchDragScrollsTheTabBar) {
		FiveTabs scene(150);
		UiDriver driver(scene.root, { 400, 400 });
		const RectF bar = scene.tabs.tabScroller().windowRect();
		driver.setPointerTool(PointerTool::Finger);
		driver.ui().setInteractionMode(InteractionMode::Touch);
		driver.drag(bar.center() + Vec2F{ 50, 0 }, bar.center() - Vec2F{ 30, 0 }, 6);
		driver.settle();
		CHECK_CLOSE(80.0f, scene.tabs.tabScroller().offset(), 2.0f);
		CHECK(!scene.tabs.tabScroller().scrollBar().isVisible());
		CHECK_EQUAL(0, scene.tabs.currentIndex());
	}

	TEST(SelectingATabRevealsIt) {
		FiveTabs scene(150);
		UiDriver driver(scene.root, { 400, 400 });
		scene.tabs.setCurrentIndex(4);
		driver.settle();
		const RectF bar = scene.tabs.tabScroller().windowRect();
		const RectF last = scene.tabs.tabButton(4).windowRect();
		CHECK(last.pos.x >= bar.pos.x - 0.5f);
		CHECK(last.pos.x + last.size.x <= bar.pos.x + bar.size.x + 0.5f);
		driver.click(last.center());
		CHECK_EQUAL(4, scene.tabs.currentIndex());
		scene.tabs.setCurrentIndex(0);
		driver.settle();
		CHECK_CLOSE(0.0f, scene.tabs.tabScroller().offset(), 0.5f);
	}

	TEST(WideViewsDoNotScroll) {
		FiveTabs scene(500);
		UiDriver driver(scene.root, { 600, 400 });
		CHECK(!scene.tabs.tabScroller().isScrollable());
		driver.click(scene.tabs.tabButton(2));
		CHECK_EQUAL(2, scene.tabs.currentIndex());
	}
}
