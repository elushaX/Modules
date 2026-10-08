#include "Button.hpp"
#include "BoxLayout.hpp"
#include "Label.hpp"
#include "Probes.hpp"
#include "StackLayout.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <functional>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct PopupScene {
		Widget root;
		EventProbe trigger{ { 100, 40 }, "trigger" };
		EventProbe elsewhere{ { 100, 40 }, "elsewhere" };
		EventProbe content{ { 150, 80 }, "content" };
		EventProbe nested{ { 120, 60 }, "nested" };

		PopupScene() {
			root.emplaceLayout<ColumnLayout>(200);
			root.addChild(trigger);
			root.addChild(elsewhere);
		}

		static PopupRequest request(Widget& popupContent, Widget& owner, PopupOptions options = {}) {
			PopupRequest result;
			result.content = &popupContent;
			result.owner = &owner;
			result.anchor = owner.windowRect();
			result.options = std::move(options);
			return result;
		}
	};
}

SUITE(PopupLifecycle) {
	TEST(ClosingAllDoesNotReopenPopupsThatReopenEachOtherOnClose) {
		Widget root;
		Label first("first");
		Label second("second");
		UiDriver driver(root, { 400, 300 });
		Ui& ui = driver.ui();

		std::function<void(Widget*, Widget*)> open = [&](Widget* content, Widget* other) {
			PopupRequest request;
			request.content = content;
			request.anchor = makeRect(10, 10, 10, 10);
			request.options.closeOnOutsidePress = false;
			request.options.onClosed = [&open, content, other]() { open(other, content); };
			ui.openPopup(request);
		};
		open(&first, &second);
		ui.closePopup(first);
		CHECK(ui.isPopupOpen(second));

		ui.closeAllPopups();
		CHECK_EQUAL(0u, ui.openPopupCount());
	}

	TEST(OpenedPopupIsLaidOutAtAnchorAndAttached) {
		PopupScene scene;
		UiDriver driver(scene.root);

		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.layout();

		CHECK(driver.ui().isPopupOpen(scene.content));
		CHECK(scene.content.ui() == &driver.ui());
		CHECK_CLOSE(0, scene.content.windowRect().pos.x, 0.01f);
		CHECK_CLOSE(42, scene.content.windowRect().pos.y, 0.01f);
		CHECK_CLOSE(150, scene.content.size().x, 0.01f);
		CHECK_CLOSE(80, scene.content.size().y, 0.01f);
	}

	TEST(PopupIsHitBeforeTreeBelow) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		driver.click(driver.centerOf(scene.content));

		CHECK_EQUAL(1, scene.content.count(Event::Type::Click));
		CHECK(driver.ui().isPopupOpen(scene.content));
	}

	TEST(PressingOutsideClosesPopupAndStillReachesTarget) {
		PopupScene scene;
		bool closedCallback = false;
		PopupOptions options;
		options.onClosed = [&closedCallback]() { closedCallback = true; };

		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger, options));

		driver.click(driver.centerOf(scene.elsewhere));

		CHECK(!driver.ui().isPopupOpen(scene.content));
		CHECK(scene.content.ui() == nullptr);
		CHECK(closedCallback);
		CHECK_EQUAL(1, scene.elsewhere.count(Event::Type::Click));
	}

	TEST(PressingOwnerDoesNotDismissSoOwnerCanToggle) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		driver.moveTo(scene.trigger);
		driver.press();

		CHECK(driver.ui().isPopupOpen(scene.content));
	}

	TEST(PopupWithoutOutsideDismissStaysOpen) {
		PopupScene scene;
		PopupOptions options;
		options.closeOnOutsidePress = false;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger, options));

		driver.click(driver.centerOf(scene.elsewhere));

		CHECK(driver.ui().isPopupOpen(scene.content));
	}

	TEST(EscapeClosesTopmostPopupOnly) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.layout();
		driver.ui().openPopup(scene.request(scene.nested, scene.content));

		driver.key(InputID::ESCAPE);

		CHECK(driver.ui().isPopupOpen(scene.content));
		CHECK(!driver.ui().isPopupOpen(scene.nested));
	}

	TEST(ReopeningSamePopupDoesNotDuplicateIt) {
		PopupScene scene;
		UiDriver driver(scene.root);

		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		CHECK_EQUAL(1u, driver.ui().openPopupCount());
	}

	TEST(PaintsPopupsAboveTree) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.layout();
		scene.content.paints = 0;

		RecordingPainter& painter = driver.paint();

		CHECK_EQUAL(1, scene.content.paints);
		CHECK(painter.isBalanced());
		CHECK_CLOSE(42, painter.commands().back().rect.pos.y, 0.01f);
	}
}

SUITE(PopupAnimation) {
	TEST(OpenedPopupGrowsOutOfItsAnchor) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.setAutoSettle(false);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.ui().updateLayout();

		const RectF anchor = scene.trigger.windowRect();
		CHECK_CLOSE(anchor.pos.y + anchor.size.y, scene.content.windowRect().pos.y, 0.01f);
		CHECK_CLOSE(0, scene.content.size().y, 0.01f);

		driver.advance(48);
		CHECK(scene.content.size().y > 0);
		CHECK(scene.content.size().y < scene.content.targetRect().size.y);

		driver.settle();
		CHECK(scene.content.rect() == scene.content.targetRect());
	}

	TEST(GrowingPopupClipsChildrenToItsAnimatedRect) {
		PopupScene scene;
		Widget popup;
		popup.emplaceLayout<ColumnLayout>(0);
		popup.addChild(scene.content);
		UiDriver driver(scene.root);
		driver.setAutoSettle(false);
		driver.ui().openPopup(scene.request(popup, scene.trigger));
		driver.ui().updateLayout();
		driver.advance(48);

		const RectF animated = popup.windowRect();
		CHECK(animated.size.y < popup.targetRect().size.y);

		RecordingPainter& painter = driver.paint();
		bool contentPainted = false;
		for (const PaintCommand& command : painter.commands()) {
			if (command.rect.size.y != scene.content.size().y) continue;
			contentPainted = true;
			CHECK_CLOSE(animated.pos.y, command.clip.pos.y, 0.01f);
			CHECK_CLOSE(animated.size.y, command.clip.size.y, 0.01f);
		}
		CHECK(contentPainted);
		CHECK(painter.isBalanced());
	}

	TEST(ReopenedPopupGrowsAgain) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.settle();
		driver.ui().closePopup(scene.content);
		driver.setAutoSettle(false);

		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.ui().updateLayout();

		CHECK_CLOSE(0, scene.content.size().y, 0.01f);
	}
}

SUITE(PopupChains) {
	TEST(OpeningFromPopupKeepsParentOpen) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.layout();

		driver.ui().openPopup(scene.request(scene.nested, scene.content));

		CHECK_EQUAL(2u, driver.ui().openPopupCount());
	}

	TEST(OpeningFromTreeReplacesUnrelatedPopups) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		driver.ui().openPopup(scene.request(scene.nested, scene.elsewhere));

		CHECK(!driver.ui().isPopupOpen(scene.content));
		CHECK(driver.ui().isPopupOpen(scene.nested));
	}

	TEST(ClosingParentClosesChildren) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.layout();
		driver.ui().openPopup(scene.request(scene.nested, scene.content));

		driver.ui().closePopup(scene.content);

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}

	TEST(ClickElsewhereInParentClosesOnlyChild) {
		PopupScene scene;
		Widget menu;
		menu.emplaceLayout<ColumnLayout>();
		EventProbe submenuItem({ 150, 30 }, "submenu item");
		EventProbe plainItem({ 150, 30 }, "plain item");
		menu.addChild(submenuItem);
		menu.addChild(plainItem);

		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(menu, scene.trigger));
		driver.layout();
		PopupOptions sideways;
		sideways.placement = Placement::Right;
		driver.ui().openPopup(scene.request(scene.nested, submenuItem, sideways));

		driver.click(submenuItem);
		CHECK(driver.ui().isPopupOpen(scene.nested));

		driver.click(plainItem);
		CHECK(driver.ui().isPopupOpen(menu));
		CHECK(!driver.ui().isPopupOpen(scene.nested));
		CHECK_EQUAL(1, plainItem.count(Event::Type::Click));
	}

	TEST(ClosingChainFromDeepestClosesWholeChain) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));
		driver.layout();
		driver.ui().openPopup(scene.request(scene.nested, scene.content));

		driver.ui().closePopupChain(scene.nested);

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
	}
}

SUITE(ActiveWidget) {
	TEST(PressingMakesWidgetAndAncestorsActive) {
		PopupScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.trigger);

		CHECK(driver.ui().activeWidget() == &scene.trigger);
		CHECK(scene.trigger.containsActiveWidget());
		CHECK(scene.root.containsActiveWidget());
		CHECK(!scene.elsewhere.containsActiveWidget());
	}

	TEST(PressingInsidePopupKeepsItsOwnerActive) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		driver.click(scene.content);

		CHECK(driver.ui().activeWidget() == &scene.content);
		CHECK(scene.trigger.containsActiveWidget());
		CHECK(scene.root.containsActiveWidget());
		CHECK(!scene.elsewhere.containsActiveWidget());
	}

	TEST(DetachingActiveSubtreeClearsFlags) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.trigger);

		scene.root.removeChild(scene.trigger);

		CHECK(!scene.trigger.containsActiveWidget());
		CHECK(scene.root.containsActiveWidget());
	}
}

SUITE(PopupOwnership) {
	TEST(RemovingOwnerClosesItsPopup) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		scene.root.removeChild(scene.trigger);

		CHECK(!driver.ui().isPopupOpen(scene.content));
	}

	TEST(HidingOwnerClosesItsPopup) {
		PopupScene scene;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger));

		scene.trigger.setVisible(false);

		CHECK(!driver.ui().isPopupOpen(scene.content));
	}

	TEST(DestroyingOpenPopupContentIsSafe) {
		PopupScene scene;
		UiDriver driver(scene.root);
		{
			EventProbe temporary({ 50, 50 }, "temporary");
			driver.ui().openPopup(scene.request(temporary, scene.trigger));
			driver.layout();
		}

		CHECK_EQUAL(0u, driver.ui().openPopupCount());
		driver.paint();
	}
}

SUITE(ModalPopups) {
	TEST(ModalBlocksTreeInput) {
		PopupScene scene;
		PopupOptions options;
		options.modal = true;
		options.closeOnOutsidePress = false;
		UiDriver driver(scene.root);
		driver.ui().openPopup(scene.request(scene.content, scene.trigger, options));

		driver.click(driver.centerOf(scene.elsewhere));

		CHECK_EQUAL(0, scene.elsewhere.count(Event::Type::Click));
		CHECK(driver.ui().isPopupOpen(scene.content));
		CHECK(driver.ui().widgetAt(driver.centerOf(scene.elsewhere)) == nullptr);
	}
}

SUITE(Tooltips) {
	TEST(TooltipAppearsAfterDwellAndHidesOnMove) {
		PopupScene scene;
		scene.trigger.setTooltip("Opens the thing");
		UiDriver driver(scene.root);

		driver.moveTo(scene.trigger);
		driver.advance(driver.ui().theme().metrics.tooltipDelayMs / 2);
		CHECK(driver.ui().openTooltip() == nullptr);
		CHECK(driver.ui().needsFrame());

		driver.advance(driver.ui().theme().metrics.tooltipDelayMs);
		Widget* tooltip = driver.ui().openTooltip();
		REQUIRE CHECK(tooltip != nullptr);
		CHECK(driver.paint().hasText("Opens the thing"));
		CHECK(tooltip->windowRect().pos.y >= scene.trigger.windowRect().pos.y + scene.trigger.size().y);

		driver.moveTo(driver.centerOf(scene.trigger) + Vec2F(1.0f, 0.0f));
		CHECK(driver.ui().openTooltip() == nullptr);
	}

	TEST(LongPressShowsTheTooltipUntilTheNextPress) {
		PopupScene scene;
		Button button("Hold me");
		scene.root.addChild(button);
		button.setTooltip("Opens the thing");
		int activations = 0;
		button.setOnActivate([&activations]() { activations++; });
		UiDriver driver(scene.root);

		driver.moveTo(button);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		REQUIRE CHECK(driver.ui().openTooltip() != nullptr);
		driver.moveTo(driver.centerOf(button) + Vec2F(2.0f, 0.0f));
		driver.release();
		CHECK(driver.ui().openTooltip() != nullptr);
		CHECK(driver.paint().hasText("Opens the thing"));
		CHECK_EQUAL(0, activations);

		driver.click(scene.elsewhere);
		CHECK(driver.ui().openTooltip() == nullptr);
	}

	TEST(LongPressHandlersStillWinOverTheTooltip) {
		PopupScene scene;
		Button button("Hold me");
		scene.root.addChild(button);
		button.setTooltip("Opens the thing");
		bool held = false;
		button.setOnLongPress([&held]() { held = true; });
		UiDriver driver(scene.root);

		driver.moveTo(button);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		driver.release();
		CHECK(held);
		CHECK(driver.ui().openTooltip() == nullptr);
	}

	TEST(TooltipIsInheritedFromNearestAncestor) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		Widget group;
		group.emplaceLayout<StackLayout>();
		group.setTooltip("group help");
		EventProbe child;
		group.addChild(child);
		root.addChild(group);
		UiDriver driver(root, { 200, 200 });

		driver.hoverFor(child, 2000);

		CHECK(driver.ui().openTooltip() != nullptr);
		CHECK(driver.paint().hasText("group help"));
	}

	TEST(TooltipDoesNotStealInput) {
		PopupScene scene;
		scene.trigger.setTooltip("help");
		UiDriver driver(scene.root);
		driver.hoverFor(scene.trigger, 2000);

		Widget* tooltip = driver.ui().openTooltip();
		REQUIRE CHECK(tooltip != nullptr);
		CHECK(driver.ui().widgetAt(tooltip->windowRect().center()) != tooltip);
	}

	TEST(NoTooltipWithoutText) {
		PopupScene scene;
		UiDriver driver(scene.root);

		driver.hoverFor(scene.trigger, 5000);
		driver.settle();

		CHECK(driver.ui().openTooltip() == nullptr);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(PressHidesTooltip) {
		PopupScene scene;
		scene.trigger.setTooltip("help");
		UiDriver driver(scene.root);
		driver.hoverFor(scene.trigger, 2000);

		driver.press();

		CHECK(driver.ui().openTooltip() == nullptr);
	}
}
