#include "Event.hpp"

#include "Widget.hpp"

using namespace tp;
using namespace tp::ui;

Event Event::pointer(Type type, Vec2F windowPosition, Vec2F delta, PointerButton button) {
	Event event(type);
	event.mWindowPosition = windowPosition;
	event.mDelta = delta;
	event.mButton = button;
	return event;
}

Event Event::wheel(Vec2F windowPosition, halnf amount, halnf horizontalAmount) {
	Event event(Type::Wheel);
	event.mWindowPosition = windowPosition;
	event.mWheelAmount = amount;
	event.mHorizontalWheelAmount = horizontalAmount;
	return event;
}

Event Event::key(Type type, InputID key) {
	Event event(type);
	event.mKey = key;
	return event;
}

Event Event::keyRepeat(InputID key) {
	Event event(Type::KeyDown);
	event.mKey = key;
	event.mRepeat = true;
	return event;
}

Event Event::text(std::string utf8) {
	Event event(Type::TextInput);
	event.mText = std::move(utf8);
	return event;
}

Event Event::gesture(const TouchGesture& gesture) {
	Event event(Type::Gesture);
	event.mWindowPosition = gesture.center;
	event.mDelta = gesture.pan;
	event.mGesture = gesture;
	return event;
}

Event& Event::withModifiers(const Modifiers& modifiers) {
	mModifiers = modifiers;
	return *this;
}

Event& Event::withPressure(halnf pressure) {
	mPressure = pressure;
	return *this;
}

Event& Event::withSample(const PointerSample& sample) {
	mSample = sample;
	mPressure = sample.pressure;
	return *this;
}

Event& Event::withClickCount(int clickCount) {
	mClickCount = clickCount;
	return *this;
}

Event& Event::withButtonsDown(bool primary, bool secondary, bool middle) {
	mButtonsDown[(int) PointerButton::Primary] = primary;
	mButtonsDown[(int) PointerButton::Secondary] = secondary;
	mButtonsDown[(int) PointerButton::Middle] = middle;
	return *this;
}

bool Event::isPointerEvent() const { return !isKeyEvent(); }

Vec2F Event::position() const {
	if (!mReceiver) return mWindowPosition;
	return mReceiver->toLocal(mWindowPosition);
}

bool Event::isDown(PointerButton button) const { return mButtonsDown[(int) button]; }
