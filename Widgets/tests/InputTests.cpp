#include "BoxLayout.hpp"
#include "Toggles.hpp"
#include "Button.hpp"
#include "Probes.hpp"
#include "StackLayout.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct TwoProbeScene {
		Widget root;
		EventProbe left{ { 100, 100 }, "left" };
		EventProbe right{ { 100, 100 }, "right" };

		TwoProbeScene() {
			root.emplaceLayout<RowLayout>();
			root.addChild(left);
			root.addChild(right);
		}
	};

	class CapturingProbe : public EventProbe {
	public:
		using EventProbe::EventProbe;

	protected:
		void onEvent(Event& event) override {
			EventProbe::onEvent(event);
			if (event.type() == Event::Type::PointerDown) ui()->capturePointer(*this);
		}
	};
}

SUITE(HitTesting) {
	TEST(DeepestVisibleWidgetUnderPointerWins) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		CHECK(driver.ui().widgetAt({ 50, 50 }) == &scene.left);
		CHECK(driver.ui().widgetAt({ 150, 50 }) == &scene.right);
		CHECK(driver.ui().widgetAt({ 500, 50 }) == nullptr);
	}

	TEST(LaterSiblingIsOnTopInStack) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		EventProbe below;
		EventProbe above;
		root.addChild(below);
		root.addChild(above);
		UiDriver driver(root, { 100, 100 });

		CHECK(driver.ui().widgetAt({ 10, 10 }) == &above);

		below.bringToFront();
		CHECK(driver.ui().widgetAt({ 10, 10 }) == &below);
	}

	TEST(HiddenWidgetsAreNotHit) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		scene.left.setVisible(false);
		driver.layout();

		CHECK(driver.ui().widgetAt({ 50, 50 }) == &scene.right);
	}
}

SUITE(HoverTracking) {
	TEST(EnterAndLeaveAreDeliveredOncePerTransition) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.moveTo({ 50, 50 });
		driver.moveTo({ 60, 50 });
		CHECK(scene.left.isHovered());
		CHECK_EQUAL(1, scene.left.hoverChanges);

		driver.moveTo({ 150, 50 });
		CHECK(!scene.left.isHovered());
		CHECK(scene.right.isHovered());
		CHECK_EQUAL(2, scene.left.hoverChanges);
		CHECK_EQUAL(1, scene.right.hoverChanges);
	}

	TEST(AncestorsStayHoveredWhileMovingBetweenChildren) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.moveTo({ 50, 50 });
		CHECK(scene.root.isHovered());
		driver.moveTo({ 150, 50 });
		CHECK(scene.root.isHovered());

		driver.moveTo({ 900, 900 });
		CHECK(!scene.root.isHovered());
	}

	TEST(HoverFollowsLayoutChangesUnderStillPointer) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });
		driver.moveTo({ 50, 50 });

		scene.left.setVisible(false);
		driver.layout();

		CHECK(!scene.left.isHovered());
		CHECK(scene.right.isHovered());
	}
}

SUITE(PressedState) {
	TEST(PressedFollowsPointerAndClearsOnRelease) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Button button("Press");
		Box other({ 100, 40 });
		root.addChild(button);
		root.addChild(other);
		UiDriver driver(root);

		driver.moveTo(button);
		driver.press();
		CHECK(button.isPressed());

		driver.moveTo(other);
		CHECK(!button.isPressed());

		driver.moveTo(button);
		CHECK(button.isPressed());

		driver.release();
		CHECK(!button.isPressed());
		driver.settle();
		CHECK(!driver.ui().needsFrame());
	}

	TEST(ReleasingOutsideLeavesNoPressedOrHoveredControl) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Button button("Press");
		Box other({ 100, 40 });
		root.addChild(button);
		root.addChild(other);
		UiDriver driver(root);

		driver.moveTo(button);
		driver.press();
		driver.moveTo(Vec2F(-50.0f, -50.0f));
		driver.release();

		CHECK(!button.isPressed());
		CHECK(!button.isHovered());
	}

	TEST(HidingPressedWidgetClearsPressedState) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Button button("Press");
		root.addChild(button);
		UiDriver driver(root);

		driver.moveTo(button);
		driver.press();
		button.setVisible(false);
		button.setVisible(true);

		CHECK(!button.isPressed());
		driver.release();
	}

	TEST(ControlRestsAfterHoverLeaves) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Button button("Hover");
		Box other({ 100, 40 });
		root.addChild(button);
		root.addChild(other);
		UiDriver driver(root);
		driver.settle();
		const RGBA resting = driver.paint().commands().front().color;

		driver.moveTo(button);
		driver.settle();
		driver.moveTo(Vec2F(-50.0f, -50.0f));
		driver.settle();

		CHECK(!button.isHovered());
		const RGBA after = driver.paint().commands().front().color;
		CHECK_CLOSE(resting.r, after.r, 0.002f);
		CHECK_CLOSE(resting.a, after.a, 0.002f);
	}
}

SUITE(EventDelivery) {
	TEST(EventsCarryReceiverLocalCoordinates) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.moveTo({ 130, 40 });
		driver.press();

		CHECK_EQUAL(1, scene.right.count(Event::Type::PointerDown));
		CHECK_CLOSE(30, scene.right.localPositions.back().x, 0.01f);
		CHECK_CLOSE(40, scene.right.localPositions.back().y, 0.01f);
	}

	TEST(UnacceptedEventsBubbleToParent) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		EventProbe parent({ 100, 100 }, "parent");
		parent.emplaceLayout<StackLayout>();
		EventProbe child({ 50, 50 }, "child");
		child.acceptEvents = false;
		parent.addChild(child);
		root.addChild(parent);
		UiDriver driver(root, { 100, 100 });

		driver.moveTo({ 10, 10 });
		driver.press();

		CHECK_EQUAL(1, child.count(Event::Type::PointerDown));
		CHECK_EQUAL(1, parent.count(Event::Type::PointerDown));
	}

	TEST(AcceptedEventsStopBubbling) {
		Widget root;
		root.emplaceLayout<StackLayout>();
		EventProbe parent({ 100, 100 }, "parent");
		parent.emplaceLayout<StackLayout>();
		EventProbe child({ 50, 50 }, "child");
		parent.addChild(child);
		root.addChild(parent);
		UiDriver driver(root, { 100, 100 });

		driver.moveTo({ 10, 10 });
		driver.press();

		CHECK_EQUAL(1, child.count(Event::Type::PointerDown));
		CHECK_EQUAL(0, parent.count(Event::Type::PointerDown));
	}

	TEST(DisabledWidgetsDoNotReceiveEvents) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });
		scene.left.setEnabled(false);

		driver.click({ 50, 50 });

		CHECK_EQUAL(0, scene.left.count(Event::Type::PointerDown));
		CHECK_EQUAL(0, scene.left.count(Event::Type::Click));
	}

	TEST(ClickRequiresPressAndReleaseOnSameWidget) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.click({ 50, 50 });
		CHECK_EQUAL(1, scene.left.count(Event::Type::Click));

		driver.moveTo({ 50, 50 });
		driver.press();
		driver.moveTo({ 150, 50 });
		driver.release();
		CHECK_EQUAL(1, scene.left.count(Event::Type::Click));
		CHECK_EQUAL(0, scene.right.count(Event::Type::Click));
	}

	TEST(SecondaryButtonDoesNotClick) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.moveTo({ 50, 50 });
		driver.press(PointerButton::Secondary);
		driver.release(PointerButton::Secondary);

		CHECK_EQUAL(1, scene.left.count(Event::Type::PointerDown));
		CHECK_EQUAL(0, scene.left.count(Event::Type::Click));
	}

	TEST(WheelGoesToHoveredWidget) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.moveTo({ 150, 50 });
		driver.wheel(1);

		CHECK_EQUAL(1, scene.right.count(Event::Type::Wheel));
		CHECK_EQUAL(0, scene.left.count(Event::Type::Wheel));
	}
}

SUITE(PointerCapture) {
	TEST(CapturedWidgetReceivesMovesOutsideItsBounds) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		CapturingProbe dragged({ 100, 100 }, "dragged");
		EventProbe other({ 100, 100 }, "other");
		root.addChild(dragged);
		root.addChild(other);
		UiDriver driver(root, { 200, 100 });

		driver.moveTo({ 50, 50 });
		driver.press();
		driver.moveTo({ 150, 50 });
		driver.moveTo({ 180, 90 });

		CHECK(driver.ui().pointerCapture() == &dragged);
		CHECK_EQUAL(0, other.count(Event::Type::PointerMove));
		CHECK(dragged.count(Event::Type::PointerMove) >= 2);
		CHECK_CLOSE(180, dragged.localPositions.back().x, 0.01f);

		driver.release();
		CHECK(driver.ui().pointerCapture() == nullptr);
		CHECK(other.isHovered());
	}

	TEST(OnlyCapturingWidgetCanRelease) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.ui().capturePointer(scene.left);
		driver.ui().releasePointer(scene.right);
		CHECK(driver.ui().pointerCapture() == &scene.left);

		driver.ui().releasePointer(scene.left);
		CHECK(driver.ui().pointerCapture() == nullptr);
	}

	TEST(DetachingCapturedWidgetClearsCapture) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.ui().capturePointer(scene.left);
		scene.root.removeChild(scene.left);

		CHECK(driver.ui().pointerCapture() == nullptr);
	}
}

SUITE(KeyboardFocus) {
	TEST(PressingFocusableWidgetFocusesIt) {
		TwoProbeScene scene;
		scene.left.focusable = true;
		UiDriver driver(scene.root, { 200, 100 });

		driver.click({ 50, 50 });
		CHECK(scene.left.hasKeyboardFocus());

		driver.click({ 150, 50 });
		CHECK(!scene.left.hasKeyboardFocus());
	}

	TEST(KeysGoToFocusedWidget) {
		TwoProbeScene scene;
		scene.left.focusable = true;
		UiDriver driver(scene.root, { 200, 100 });
		driver.click({ 50, 50 });

		driver.key(InputID::ENTER);

		CHECK_EQUAL(1, scene.left.count(Event::Type::KeyDown));
		CHECK_EQUAL(1, scene.left.count(Event::Type::KeyUp));
		CHECK_EQUAL(0, scene.right.count(Event::Type::KeyDown));
	}

	TEST(HeldKeysAreTracked) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		driver.ui().dispatch(Event::key(Event::Type::KeyDown, InputID::SPACE));
		CHECK(driver.ui().isKeyDown(InputID::SPACE));
		driver.ui().dispatch(Event::key(Event::Type::KeyUp, InputID::SPACE));
		CHECK(!driver.ui().isKeyDown(InputID::SPACE));
	}
}

SUITE(TickingAndFrames) {
	TEST(TickingWidgetsReceiveTicksUntilStopped) {
		Widget root;
		EventProbe probe;
		root.addChild(probe);
		UiDriver driver(root);

		probe.beginTicking();
		driver.advance(48);
		CHECK_EQUAL(3, probe.ticks);
		CHECK(driver.ui().needsFrame());

		probe.endTicking();
		driver.paint();
		driver.advance(48);
		CHECK_EQUAL(3, probe.ticks);
		CHECK(!driver.ui().needsFrame());
	}

	TEST(DetachedWidgetsStopTicking) {
		Widget root;
		EventProbe probe;
		root.addChild(probe);
		UiDriver driver(root);

		probe.beginTicking();
		root.removeChild(probe);
		driver.advance(48);

		CHECK_EQUAL(0, probe.ticks);
	}

	TEST(TickingResumesWhenReattached) {
		Widget root;
		EventProbe probe;
		probe.beginTicking();
		UiDriver driver(root);

		root.addChild(probe);
		driver.advance(16);

		CHECK_EQUAL(1, probe.ticks);
	}

	TEST(IdleUiNeedsNoFrames) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });
		driver.settle();

		CHECK(!driver.ui().needsFrame());
	}
}

SUITE(FrameTiming) {
	TEST(LongIdleFrameStillAnimatesToggles) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Checkbox checkbox("Check");
		root.addChild(checkbox);
		UiDriver driver(root);
		driver.setAutoSettle(false);
		driver.moveTo(checkbox);
		driver.settle();

		driver.press();
		driver.release();
		driver.ui().advance(3000);

		CHECK(driver.ui().needsFrame());
		driver.settle();
		CHECK(!driver.ui().needsFrame());
	}

	TEST(WindowResizeAppliesLayoutImmediately) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box fill({ 10, 10 });
		fill.setSizing(AxisSizing::fill(), AxisSizing::fill());
		root.addChild(fill);
		UiDriver driver(root, { 400, 300 });
		driver.setAutoSettle(false);
		driver.settle();

		driver.ui().resizeViewport(makeRect(0, 0, 900, 600));

		CHECK_CLOSE(900, root.rect().size.x, 0.01f);
		CHECK_CLOSE(900, fill.rect().size.x, 0.01f);
		CHECK_CLOSE(600, fill.rect().size.y, 0.01f);
		CHECK(!driver.ui().isLayoutPending());
	}

	TEST(InternalLayoutChangesStillAnimateAfterResize) {
		Widget root;
		root.emplaceLayout<RowLayout>();
		Box first({ 100, 50 });
		root.addChild(first);
		UiDriver driver(root, { 400, 300 });
		driver.setAutoSettle(false);
		driver.ui().resizeViewport(makeRect(0, 0, 900, 600));

		first.setPreferred({ 300, 50 });
		driver.ui().updateLayout();
		driver.advance(16);

		CHECK(first.size().x > 100 && first.size().x < 300);
	}
}

SUITE(Painting) {
	TEST(PaintTraversesVisibleTreeWithTranslationAndClip) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });

		RecordingPainter& painter = driver.paint();

		CHECK_EQUAL(1, scene.left.paints);
		CHECK_EQUAL(1, scene.right.paints);
		CHECK(painter.isBalanced());

		const auto& commands = painter.commands();
		CHECK_EQUAL(2u, commands.size());
		CHECK_CLOSE(100, commands[1].rect.pos.x, 0.01f);
		CHECK_CLOSE(0, commands[1].clip.pos.x, 0.01f);
		CHECK_CLOSE(200, commands[1].clip.size.x, 0.01f);
	}

	TEST(HiddenWidgetsAreNotPainted) {
		TwoProbeScene scene;
		UiDriver driver(scene.root, { 200, 100 });
		scene.right.setVisible(false);

		driver.paint();

		CHECK_EQUAL(0, scene.right.paints);
	}

	TEST(DumpTreeShowsNamesAndState) {
		TwoProbeScene scene;
		scene.root.setDebugName("root");
		UiDriver driver(scene.root, { 200, 100 });
		driver.moveTo({ 50, 50 });

		const std::string dump = driver.dump();
		CHECK(dump.find("root") != std::string::npos);
		CHECK(dump.find("left [0, 0, 100 x 100] hovered") != std::string::npos);
	}
}

SUITE(KeyboardEvents) {
	TEST(RepeatKeepsKeyHeldAndTextReachesFocus) {
		EventProbe field{ { 100, 30 }, "field" };
		field.focusable = true;
		field.acceptEvents = true;
		Widget root;
		root.addChild(field);
		UiDriver driver(root);
		driver.ui().setKeyboardFocus(&field);

		driver.ui().dispatch(Event::key(Event::Type::KeyDown, InputID::A));
		driver.ui().dispatch(Event::keyRepeat(InputID::A));
		CHECK(driver.ui().isKeyDown(InputID::A));
		driver.ui().dispatch(Event::text("a"));

		CHECK_EQUAL(2, field.count(Event::Type::KeyDown));
		CHECK_EQUAL(1, field.count(Event::Type::TextInput));
	}
}

SUITE(UiAdvanceHook) {
	TEST(AdvanceReportsElapsedTime) {
		Widget root;
		UiDriver driver(root);
		halnf elapsed = 0;
		driver.ui().setOnAdvance([&elapsed](halnf deltaMs) { elapsed += deltaMs; });

		driver.advance(40);
		CHECK_CLOSE(40, elapsed, 0.01f);
	}
}

namespace {
	class FilteringParent : public Widget {
	public:
		bool swallowMoves = false;
		int filteredPresses = 0;

	protected:
		void filterDescendantEvent(Event& event) override {
			if (event.type() == Event::Type::PointerDown) filteredPresses++;
			if (swallowMoves && event.type() == Event::Type::PointerMove) event.accept();
		}
	};
}

SUITE(DescendantEventFilter) {
	TEST(AncestorsSeeDescendantEventsFirstAndCanConsumeThem) {
		FilteringParent parent;
		EventProbe child({ 100, 40 }, "child");
		parent.emplaceLayout<ColumnLayout>();
		parent.addChild(child);
		UiDriver driver(parent);

		driver.click(child);
		CHECK_EQUAL(1, parent.filteredPresses);
		CHECK_EQUAL(1, child.count(Event::Type::Click));

		const int movesBefore = child.count(Event::Type::PointerMove);
		parent.swallowMoves = true;
		driver.moveTo(driver.centerOf(child) + Vec2F(2, 0));
		CHECK_EQUAL(movesBefore, child.count(Event::Type::PointerMove));
	}
}

SUITE(LongPress) {
	struct ButtonScene {
		Widget root;
		Button button{ "Hold" };
		int clicks = 0;
		int longPresses = 0;

		ButtonScene() {
			root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
			button.setOnActivate([this]() { clicks++; });
			button.setOnLongPress([this]() { longPresses++; });
			root.addChild(button);
		}
	};

	TEST(HoldingFiresLongPressOnceAndSuppressesTheClick) {
		ButtonScene scene;
		UiDriver driver(scene.root);
		driver.moveTo(scene.button);
		driver.press();

		CHECK(driver.ui().needsFrame());
		driver.advance(Ui::kLongPressMs + 20);
		CHECK_EQUAL(1, scene.longPresses);
		driver.advance(Ui::kLongPressMs + 20);
		driver.release();

		CHECK_EQUAL(1, scene.longPresses);
		CHECK_EQUAL(0, scene.clicks);
	}

	TEST(SlowFramesStillFireLongPressOnTime) {
		ButtonScene scene;
		UiDriver driver(scene.root);
		driver.moveTo(scene.button);
		driver.press();

		driver.ui().advance(Ui::kLongPressMs + 20);

		CHECK_EQUAL(1, scene.longPresses);
		driver.release();
	}

	TEST(QuickTapClicksWithoutLongPress) {
		ButtonScene scene;
		UiDriver driver(scene.root);

		driver.moveTo(scene.button);
		driver.press();
		driver.advance(100);
		driver.release();
		driver.advance(Ui::kLongPressMs * 2);

		CHECK_EQUAL(1, scene.clicks);
		CHECK_EQUAL(0, scene.longPresses);
	}

	TEST(MovingAwayCancelsTheLongPress) {
		ButtonScene scene;
		UiDriver driver(scene.root);
		const Vec2F center = driver.centerOf(scene.button);
		driver.moveTo(center);
		driver.press();

		driver.moveTo(center + Vec2F(20.0f, 0.0f));
		driver.advance(Ui::kLongPressMs + 20);

		CHECK_EQUAL(0, scene.longPresses);
	}

	TEST(WithoutAHandlerHoldingStillClicks) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Start);
		int clicks = 0;
		Button plain{ "Plain", [&clicks]() { clicks++; } };
		root.addChild(plain);
		UiDriver driver(root);

		driver.moveTo(plain);
		driver.press();
		driver.advance(Ui::kLongPressMs + 20);
		driver.release();

		CHECK_EQUAL(1, clicks);
	}

	TEST(IdleUiDoesNotKeepRequestingFrames) {
		ButtonScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.button);
		driver.settle();

		CHECK(!driver.ui().needsFrame());
	}
}
