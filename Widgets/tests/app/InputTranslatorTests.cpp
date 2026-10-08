#include "InputTranslator.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

namespace {
	struct TranslatorScene {
		EventHandler input;
		InputTranslator translator;

		std::vector<Event> keyEvents(InputRouting routing = {}) {
			std::vector<Event> keys;
			for (Event& event : translator.translate(input, routing)) {
				if (event.isKeyEvent()) keys.push_back(event);
			}
			return keys;
		}
	};
}

SUITE(InputTranslatorKeys) {
	TEST(EveryKeyIsDeliveredWithItsCode) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::K, true);
		scene.translator.onKey(InputID::F6, true);
		scene.translator.onKey(InputID::KP_ENTER, true);

		const std::vector<Event> events = scene.keyEvents();

		CHECK_EQUAL(3u, events.size());
		CHECK(events[0].keyCode() == InputID::K);
		CHECK(events[1].keyCode() == InputID::F6);
		CHECK(events[2].keyCode() == InputID::KP_ENTER);
		CHECK(events[0].type() == Event::Type::KeyDown);
	}

	TEST(PressAndReleaseInOneFrameAreBothDelivered) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::Z, true);
		scene.translator.onKey(InputID::Z, false);

		const std::vector<Event> events = scene.keyEvents();

		CHECK_EQUAL(2u, events.size());
		CHECK(events[0].type() == Event::Type::KeyDown);
		CHECK(events[1].type() == Event::Type::KeyUp);
	}

	TEST(ModifiersFollowEitherSide) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::RIGHT_CONTROL, true);
		scene.translator.onKey(InputID::LEFT_SHIFT, true);
		scene.translator.onKey(InputID::Z, true);

		const Event& undo = scene.keyEvents().back();

		CHECK(undo.modifiers().control);
		CHECK(undo.modifiers().shift);
		CHECK(!undo.modifiers().alt);
	}

	TEST(ReleasingOneSideKeepsModifierWhileOtherSideIsDown) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::LEFT_CONTROL, true);
		scene.translator.onKey(InputID::RIGHT_CONTROL, true);
		scene.translator.onKey(InputID::LEFT_CONTROL, false);
		scene.translator.onKey(InputID::Y, true);

		CHECK(scene.keyEvents().back().modifiers().control);
	}

	TEST(RepeatIsAKeyDownMarkedAsRepeat) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::Z, true);
		scene.translator.onKeyRepeat(InputID::Z);
		scene.translator.onKeyRepeat(InputID::Z);

		const std::vector<Event> events = scene.keyEvents();

		CHECK_EQUAL(3u, events.size());
		CHECK(!events[0].isRepeat());
		CHECK(events[1].isRepeat());
		CHECK(events[2].type() == Event::Type::KeyDown);
	}

	TEST(BlockedKeyboardDropsPressesButStillReleasesDeliveredKeys) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::A, true);
		CHECK_EQUAL(1u, scene.keyEvents().size());

		InputRouting blocked;
		blocked.keyboardBlocked = true;
		scene.translator.onKey(InputID::B, true);
		scene.translator.onKeyRepeat(InputID::A);
		scene.translator.onKey(InputID::A, false);
		scene.translator.onKey(InputID::B, false);

		const std::vector<Event> events = scene.keyEvents(blocked);

		CHECK_EQUAL(1u, events.size());
		CHECK(events[0].type() == Event::Type::KeyUp);
		CHECK(events[0].keyCode() == InputID::A);
	}

	TEST(TextIsDeliveredOnlyWithoutCommandModifiers) {
		TranslatorScene scene;
		scene.translator.onText("a");
		scene.translator.onKey(InputID::LEFT_CONTROL, true);
		scene.translator.onText("z");

		std::vector<std::string> texts;
		for (const Event& event : scene.keyEvents()) {
			if (event.type() == Event::Type::TextInput) texts.push_back(event.text());
		}

		CHECK_EQUAL(1u, texts.size());
		CHECK_EQUAL("a", texts[0]);
	}

	TEST(LosingFocusClearsModifiers) {
		TranslatorScene scene;
		scene.translator.onKey(InputID::LEFT_ALT, true);
		scene.translator.onKeyboardFocus(false);
		scene.translator.onKey(InputID::Q, true);

		CHECK(!scene.keyEvents().back().modifiers().alt);
	}
}

SUITE(InputTranslatorPen) {
	void post(EventHandler & input, const PointerSample& sample) {
		InputEvent event;
		event.type = InputEvent::Type::POINTER_SAMPLE;
		event.sample = sample;
		input.postEvent(InputID::MOUSE1, event);
	}

	void move(EventHandler & input, Vec2F point) {
		input.postEvent(InputID::MOUSE1, { InputEvent::Type::MOUSE_POS, {}, point });
	}

	void press(EventHandler & input) {
		input.postEvent(InputID::MOUSE1, { InputEvent::Type::BUTTON_ACTION, InputEvent::ButtonAction::PRESS, {} });
	}

	std::vector<Event> step(TranslatorScene & scene) {
		std::vector<Event> all;
		do {
			scene.input.processEvent();
			for (Event& event : scene.translator.translate(scene.input, {}))
				all.push_back(event);
		} while (scene.input.hasQueuedEvents());
		return all;
	}

	void scroll(EventHandler & input, Vec2F delta) {
		input.postEvent(InputID::SCROLL, { InputEvent::Type::SCROLL, {}, {}, delta });
	}

	size_t wheelEvents(const std::vector<Event>& events) {
		return (size_t) std::count_if(events.begin(), events.end(), [](const Event& event) {
			return event.type() == Event::Type::Wheel;
		});
	}

	TEST(ZeroScrollLeavesTheInputQueue) {
		TranslatorScene scene;
		scroll(scene.input, { 0, 0 });
		for (int pass = 0; pass < 4 && scene.input.hasQueuedEvents(); pass++)
			scene.input.processEvent();
		CHECK(!scene.input.hasQueuedEvents());
	}

	TEST(EachScrollReachesTheUiOnce) {
		TranslatorScene scene;
		scroll(scene.input, { 0, 1 });
		scroll(scene.input, { 0, 0 });
		scroll(scene.input, { 0, -2 });
		const std::vector<Event> events = step(scene);
		CHECK_EQUAL(2u, wheelEvents(events));
		CHECK(!scene.input.hasQueuedEvents());
	}

	TEST(PenPressureIsContinuousAndCarriesTiltAndRotation) {
		TranslatorScene scene;
		PointerSample sample{ PointerTool::Pen, 0.4f, { 30, -15 }, 90 };
		post(scene.input, sample);
		move(scene.input, { 10, 10 });
		press(scene.input);

		const std::vector<Event> events = step(scene);
		REQUIRE CHECK(!events.empty());
		const Event& down = events.back();
		CHECK(down.type() == Event::Type::PointerDown);
		CHECK_CLOSE(0.4f, down.pressure(), 1e-5f);
		CHECK(down.sample().tool == PointerTool::Pen);
		CHECK_CLOSE(30, down.sample().tilt.x, 1e-5f);
		CHECK_CLOSE(-15, down.sample().tilt.y, 1e-5f);
		CHECK_CLOSE(90, down.sample().rotation, 1e-5f);
	}

	TEST(PressureChangeWithoutMotionStillReachesWidgets) {
		TranslatorScene scene;
		post(scene.input, { PointerTool::Pen, 0.2f });
		move(scene.input, { 10, 10 });
		press(scene.input);
		step(scene);

		post(scene.input, { PointerTool::Pen, 0.8f });
		const std::vector<Event> events = step(scene);
		REQUIRE CHECK_EQUAL(1u, events.size());
		CHECK(events[0].type() == Event::Type::PointerMove);
		CHECK_CLOSE(0.8f, events[0].pressure(), 1e-5f);
	}

	TEST(MousePressureFollowsThePrimaryButton) {
		TranslatorScene scene;
		move(scene.input, { 10, 10 });
		CHECK_EQUAL(0.0f, step(scene).back().pressure());
		press(scene.input);
		const Event down = step(scene).back();
		CHECK(down.sample().tool == PointerTool::Mouse);
		CHECK_EQUAL(1.0f, down.pressure());
	}

	TEST(EraserEndIsReportedAsItsOwnTool) {
		TranslatorScene scene;
		post(scene.input, { PointerTool::Eraser, 0.5f });
		move(scene.input, { 3, 3 });
		CHECK(step(scene).back().sample().tool == PointerTool::Eraser);
	}
}

SUITE(InputTranslatorGestures) {
	TEST(GesturesBecomeUiEventsInOrder) {
		InputTranslator translator;
		EventHandler input;
		translator.onGesture({ GesturePhase::Begin, { 10, 20 } });
		translator.onGesture({ GesturePhase::Update, { 15, 20 }, { 5, 0 }, 1.5f });

		const std::vector<Event> events = translator.translate(input, {});

		size_t gestures = 0;
		for (const Event& event : events) {
			if (event.type() != Event::Type::Gesture) continue;
			gestures++;
			if (gestures == 2) {
				CHECK_CLOSE(15.0f, event.windowPosition().x, 0.001f);
				CHECK_CLOSE(5.0f, event.delta().x, 0.001f);
				CHECK_CLOSE(1.5f, event.touchGesture().scale, 0.001f);
			}
		}
		CHECK_EQUAL(2u, gestures);
		CHECK(translator.translate(input, {}).size() <= 1);
	}

	TEST(BlockedPointerStillDeliversGestureEnd) {
		InputTranslator translator;
		EventHandler input;
		translator.onGesture({ GesturePhase::Update, { 1, 1 }, { 1, 0 } });
		translator.onGesture({ GesturePhase::End, { 1, 1 } });

		InputRouting routing;
		routing.pointerBlocked = true;
		const std::vector<Event> events = translator.translate(input, routing);

		size_t ends = 0, updates = 0;
		for (const Event& event : events) {
			if (event.type() != Event::Type::Gesture) continue;
			if (event.touchGesture().phase == GesturePhase::End) ends++;
			else updates++;
		}
		CHECK_EQUAL(1u, ends);
		CHECK_EQUAL(0u, updates);
	}
}
