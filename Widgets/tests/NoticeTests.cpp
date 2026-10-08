#include "BoxLayout.hpp"
#include "Notice.hpp"
#include "Probes.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

SUITE(NoticeBalloon) {
	TEST(ShowsCenteredOverAnchorAndHidesAfterDuration) {
		Widget root;
		EventProbe area({ 600, 400 }, "area");
		root.emplaceLayout<ColumnLayout>();
		root.addChild(area);
		Notice notice;
		UiDriver driver(root, { 600, 400 });

		notice.show(area, "You can't edit a locked layer", 1000);
		driver.advance(16);
		CHECK(notice.isShown());
		driver.advance(300);
		CHECK(driver.paint().hasText("You can't edit a locked layer"));
		const RectF rect = notice.windowRect();
		CHECK_CLOSE(300, rect.pos.x + rect.size.x / 2, 1.0f);

		driver.advance(300);
		CHECK(notice.isShown());
		driver.advance(600);
		CHECK(!notice.isShown());
	}

	TEST(ShowingAgainRestartsTimerAndReplacesText) {
		Widget root;
		EventProbe area({ 600, 400 }, "area");
		root.emplaceLayout<ColumnLayout>();
		root.addChild(area);
		Notice notice;
		UiDriver driver(root, { 600, 400 });

		notice.show(area, "first", 1000);
		driver.advance(800);
		notice.show(area, "second", 1000);
		driver.advance(800);
		CHECK(notice.isShown());
		CHECK_EQUAL("second", notice.text());
	}

	TEST(NoticeDoesNotBlockInput) {
		Widget root;
		EventProbe area({ 600, 400 }, "area");
		root.emplaceLayout<ColumnLayout>();
		root.addChild(area);
		Notice notice;
		UiDriver driver(root, { 600, 400 });

		notice.show(area, "message", 1000);
		driver.advance(300);
		driver.click(driver.centerOf(notice));
		CHECK_EQUAL(1, area.count(Event::Type::Click));
	}
}

SUITE(NoticeKeyboard) {
	TEST(KeysStillReachRootWhileNoticeIsShown) {
		Widget root;
		EventProbe area({ 600, 400 }, "area");
		root.emplaceLayout<ColumnLayout>();
		root.addChild(area);
		Notice notice;
		UiDriver driver(root, { 600, 400 });

		notice.show(area, "message", 1000);
		const int before = area.count(Event::Type::KeyDown);
		driver.ui().setKeyboardFocus(&area);
		driver.key(InputID::K);
		CHECK_EQUAL(before + 1, area.count(Event::Type::KeyDown));
		CHECK(notice.isShown());
	}
}
