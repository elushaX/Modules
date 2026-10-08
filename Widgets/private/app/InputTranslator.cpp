#include "InputTranslator.hpp"

#include <iterator>

using namespace tp;
using namespace tp::ui;

namespace {
	struct ButtonBinding {
		InputID input;
		PointerButton button;
	};

	const ButtonBinding kPointerButtons[] = {
		{ InputID::MOUSE1, PointerButton::Primary },
		{ InputID::MOUSE2, PointerButton::Secondary },
		{ InputID::MOUSE3, PointerButton::Middle },
	};

	PointerSample sampleOf(const EventHandler& input) {
		PointerSample sample = input.getPointerSample();
		sample.pressure = input.getPointerPressure();
		return sample;
	}

	bool isShift(InputID key) { return key == InputID::LEFT_SHIFT || key == InputID::RIGHT_SHIFT; }
	bool isControl(InputID key) { return key == InputID::LEFT_CONTROL || key == InputID::RIGHT_CONTROL; }
	bool isAlt(InputID key) { return key == InputID::LEFT_ALT || key == InputID::RIGHT_ALT; }
}

std::vector<Event> InputTranslator::translate(const EventHandler& input, const InputRouting& routing) {
	std::vector<Event> events;

	translateGestures(events, routing.pointerBlocked);
	translatePointer(input, events);
	translateButtons(input, events, routing.pointerBlocked);
	if (!routing.pointerBlocked) translateWheel(input, events);
	translateKeys(events, routing.keyboardBlocked);

	return events;
}

void InputTranslator::translatePointer(const EventHandler& input, std::vector<Event>& events) {
	const Vec2F pointer = input.getPointer();
	const PointerSample sample = sampleOf(input);
	if (mPointerKnown && pointer == mLastPointer && (!sample.isStylus() || sample == mLastSample)) return;
	mLastSample = sample;

	const Vec2F delta = mPointerKnown ? pointer - mLastPointer : Vec2F{ 0, 0 };
	events.push_back(
		Event::pointer(Event::Type::PointerMove, pointer, delta, PointerButton::Primary)
			.withModifiers(mModifiers)
			.withSample(sample)
	);

	mLastPointer = pointer;
	mPointerKnown = true;
}

void InputTranslator::translateButtons(const EventHandler& input, std::vector<Event>& events, bool pressesBlocked) {
	for (const ButtonBinding& binding : kPointerButtons) {
		const size_t index = (size_t) binding.button;
		const bool pressed = input.isPressed(binding.input) && !pressesBlocked;
		const bool released = input.isReleased(binding.input) && mButtonDelivered[index];
		if (!pressed && !released) continue;

		mButtonDelivered[index] = pressed;

		const Event::Type type = pressed ? Event::Type::PointerDown : Event::Type::PointerUp;
		events.push_back(
			Event::pointer(type, input.getPointer(), { 0, 0 }, binding.button)
				.withModifiers(mModifiers)
				.withSample(sampleOf(input))
		);
	}
}

void InputTranslator::translateWheel(const EventHandler& input, std::vector<Event>& events) {
	const halnf amount = input.getScrollY();
	const halnf horizontal = input.getScrollX();
	if (amount == 0 && horizontal == 0) return;

	events.push_back(Event::wheel(input.getPointer(), amount, horizontal).withModifiers(mModifiers));
}

void InputTranslator::translateKeys(std::vector<Event>& events, bool pressesBlocked) {
	for (PendingKey& pending : mPendingKeys) {
		const InputID key = pending.event.keyCode();
		const bool text = pending.event.type() == Event::Type::TextInput;

		if (pending.event.type() == Event::Type::KeyUp) {
			if (mDeliveredKeys.erase(key) == 0) continue;
		} else if (pressesBlocked) {
			continue;
		} else if (!text && pending.press) {
			mDeliveredKeys.insert(key);
		} else if (!text && !mDeliveredKeys.count(key)) {
			continue;
		}

		events.push_back(std::move(pending.event));
	}
	mPendingKeys.clear();
}

void InputTranslator::translateGestures(std::vector<Event>& events, bool pointerBlocked) {
	for (const TouchGesture& gesture : mPendingGestures) {
		if (pointerBlocked && gesture.phase != GesturePhase::End) continue;
		events.push_back(Event::gesture(gesture).withModifiers(mModifiers));
	}
	mPendingGestures.clear();
}

void InputTranslator::onGesture(const TouchGesture& gesture) { mPendingGestures.push_back(gesture); }

void InputTranslator::onKey(InputID key, bool pressed) {
	updateModifiers(key, pressed);
	const Event::Type type = pressed ? Event::Type::KeyDown : Event::Type::KeyUp;
	mPendingKeys.push_back({ Event::key(type, key).withModifiers(mModifiers), pressed });
}

void InputTranslator::onKeyRepeat(InputID key) {
	mPendingKeys.push_back({ Event::keyRepeat(key).withModifiers(mModifiers), false });
}

void InputTranslator::onText(const std::string& utf8) {
	if (mModifiers.control || mModifiers.alt) return;
	mPendingKeys.push_back({ Event::text(utf8).withModifiers(mModifiers), false });
}

void InputTranslator::onKeyboardFocus(bool focused) {
	if (focused) return;
	mModifiersDown.clear();
	mModifiers = {};
}

void InputTranslator::updateModifiers(InputID key, bool pressed) {
	if (!isShift(key) && !isControl(key) && !isAlt(key)) return;

	if (pressed) mModifiersDown.insert(key);
	else mModifiersDown.erase(key);

	mModifiers = {};
	for (InputID down : mModifiersDown) {
		mModifiers.shift |= isShift(down);
		mModifiers.control |= isControl(down);
		mModifiers.alt |= isAlt(down);
	}
}
