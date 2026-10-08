#include "Button.hpp"
#include "Collapsible.hpp"
#include "Slider.hpp"
#include "UiDriver.hpp"

#include "BoxLayout.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct CollapsibleScene {
		Widget root;
		Collapsible section{ "Shape" };
		LabeledSlider roundness{ "Roundness" };
		LabeledSlider angle{ "Angle", 0, 360 };
		Label after{ "After" };

		CollapsibleScene() {
			root.emplaceLayout<ColumnLayout>(4).setPadding(Insets::uniform(8));
			root.setSizing(AxisSizing::fixed(300), AxisSizing::fit());
			roundness.setSizing(AxisSizing::fill(), AxisSizing::fit());
			angle.setSizing(AxisSizing::fill(), AxisSizing::fit());
			section.body().addChild(roundness);
			section.body().addChild(angle);
			root.addChild(section);
			root.addChild(after);
		}
	};
}

SUITE(CollapsibleWidget) {
	TEST(StartsCollapsedWithBodyHidden) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		CHECK(!scene.section.isExpanded());
		CHECK(!scene.section.isBodyShown());
		CHECK_CLOSE(scene.section.header().size().y, scene.section.size().y, 0.5f);
	}

	TEST(ClickingTheHeaderExpandsAndCollapses) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		std::vector<bool> toggles;
		scene.section.setOnToggled([&toggles](bool expanded) { toggles.push_back(expanded); });
		const halnf collapsedAfter = scene.after.windowRect().pos.y;

		driver.click(scene.section.header());
		CHECK(scene.section.isExpanded());
		CHECK(scene.section.isBodyShown());
		CHECK(scene.after.windowRect().pos.y > collapsedAfter + 40);
		driver.settle();
		CHECK_CLOSE(1, scene.section.header().chevronTurn(), 0.01f);

		driver.click(scene.section.header());
		CHECK(!scene.section.isExpanded());
		CHECK_CLOSE(collapsedAfter, scene.after.windowRect().pos.y, 0.5f);
		REQUIRE CHECK_EQUAL(2u, toggles.size());
		CHECK(toggles[0] && !toggles[1]);
	}

	TEST(ExpandedSlidersFillTheWidth) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		scene.section.setExpanded(true, false);
		driver.layout();
		const RectF body = scene.section.body().windowRect();
		CHECK_CLOSE(scene.section.windowRect().size.x, body.size.x, 0.5f);
		CHECK(scene.roundness.windowRect().size.x > 230);
		CHECK_CLOSE(
			body.pos.x + body.size.x, scene.roundness.windowRect().pos.x + scene.roundness.windowRect().size.x, 0.5f
		);
	}

	TEST(OpeningRevealsTheBodyWithoutOverlappingTheNextWidget) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		driver.setAutoSettle(false);
		driver.click(scene.section.header());

		CHECK(scene.section.clipsChildren());
		bool grew = false;
		halnf lastBottom = 0;
		for (int frame = 0; frame < 40; frame++) {
			driver.layout();
			const RectF section = scene.section.windowRect();
			const halnf bottom = section.pos.y + section.size.y;
			CHECK(scene.after.windowRect().pos.y >= bottom - 0.5f);
			grew = grew || (frame > 0 && bottom > lastBottom + 0.5f);
			lastBottom = bottom;
			driver.advance(16);
		}
		CHECK(grew);
		driver.settle();
		CHECK(!scene.section.isGeometryMoving());
		const RectF angle = scene.angle.windowRect();
		CHECK(angle.pos.y + angle.size.y <= scene.section.windowRect().pos.y + scene.section.windowRect().size.y + 0.5f);
	}

	TEST(OpeningAndClosingAnimateTheHeightOverSeveralFrames) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		const halnf closed = scene.section.size().y;
		driver.setAutoSettle(false);

		driver.click(scene.section.header());
		std::vector<halnf> opening;
		for (int frame = 0; frame < 30 && scene.section.isTransitioning(); frame++) {
			driver.advance(16);
			driver.layout();
			opening.push_back(scene.section.size().y);
		}
		REQUIRE CHECK(opening.size() >= 5);
		CHECK(opening[1] > closed);
		CHECK(opening[1] < opening.back() - 10);
		for (size_t index = 1; index < opening.size(); index++)
			CHECK(opening[index] >= opening[index - 1] - 0.5f);
		const halnf open = opening.back();
		CHECK(scene.section.isBodyShown());

		driver.click(scene.section.header());
		CHECK(scene.section.isBodyShown());
		std::vector<halnf> closing;
		for (int frame = 0; frame < 30 && scene.section.isTransitioning(); frame++) {
			driver.advance(16);
			driver.layout();
			closing.push_back(scene.section.size().y);
		}
		driver.layout();
		REQUIRE CHECK(closing.size() >= 5);
		CHECK(closing[1] < open);
		CHECK(closing[1] > closed + 10);
		CHECK_CLOSE(closed, scene.section.size().y, 0.5f);
		CHECK(!scene.section.isBodyShown());
	}

	TEST(SlidersInsideStayUsable) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		driver.click(scene.section.header());
		driver.setSlider(scene.angle.slider(), 90);
		CHECK_CLOSE(90, scene.angle.value(), 3);
	}

	TEST(SummaryShowsOnTheHeader) {
		CollapsibleScene scene;
		scene.section.header().setSummary("45°");
		UiDriver driver(scene.root, { 400, 400 });
		CHECK(driver.paint().hasText("Shape"));
		CHECK(driver.paint().hasText("45°"));
	}

	TEST(KeyboardActivationToggles) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		driver.ui().setKeyboardFocus(&scene.section.header());
		driver.key(InputID::SPACE);
		CHECK(scene.section.isExpanded());
	}
}

SUITE(CollapsibleTransitions) {
	TEST(BodyNeverPaintsOverTheHeaderWhileOpening) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		driver.setAutoSettle(false);
		driver.click(scene.section.header());
		for (int frame = 0; frame < 30; frame++) {
			driver.advance(16);
			driver.layout();
			const RectF header = scene.section.header().windowRect();
			const halnf headerBottom = header.pos.y + header.size.y;
			CHECK(driver.ui().widgetAt(header.center()) == &scene.section.header());
			const RectF visibleRoundness = scene.roundness.windowRect();
			if (visibleRoundness.pos.y < headerBottom) {
				const Widget* hit = driver.ui().widgetAt({ visibleRoundness.center().x, headerBottom - 2 });
				CHECK(hit == &scene.section.header());
			}
		}
	}

	TEST(TappingTheHeaderMidTransitionNeverReachesTheItems) {
		CollapsibleScene scene;
		UiDriver driver(scene.root, { 400, 400 });
		int changes = 0;
		scene.roundness.setOnChanged([&changes](halnf) { changes++; });
		driver.setAutoSettle(false);

		driver.click(scene.section.header());
		driver.advance(48);
		driver.layout();
		driver.click(scene.section.header());
		driver.advance(48);
		driver.layout();
		driver.click(scene.section.header());
		CHECK_EQUAL(0, changes);
		CHECK(scene.section.isExpanded());
	}
}

SUITE(CollapsibleHeaderAction) {
	TEST(ActionSitsAtHeaderEndAndClicksDoNotToggle) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Collapsible section("Children");
		Label content("content");
		section.body().addChild(content);
		Button add("+ Add");
		int added = 0;
		add.setOnActivate([&] { added++; });
		section.setHeaderAction(&add);
		section.header().setSummary("3");
		root.addChild(section);
		UiDriver driver(root, { 600, 400 });

		const RectF header = section.header().windowRect();
		const RectF action = add.windowRect();
		CHECK_CLOSE(header.pos.x + header.size.x, action.pos.x + action.size.x, 0.5f);
		CHECK(action.pos.y >= header.pos.y - 0.5f && action.pos.y + action.size.y <= header.pos.y + header.size.y + 0.5f);

		driver.click(add);
		CHECK_EQUAL(1, added);
		CHECK(!section.isExpanded());

		driver.click(section.header().windowPosition() + Vec2F(30.0f, header.size.y / 2));
		CHECK(section.isExpanded());

		section.setHeaderAction(nullptr);
		CHECK(add.parent() == nullptr);
	}
}
