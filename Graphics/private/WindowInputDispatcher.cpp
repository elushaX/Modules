#include "WindowInputDispatcher.hpp"

#include <algorithm>

using namespace tp;

void WindowInputDispatcher::addObserver(WindowInputObserver* observer) {
	if (std::find(mObservers.begin(), mObservers.end(), observer) == mObservers.end()) mObservers.push_back(observer);
}

void WindowInputDispatcher::removeObserver(WindowInputObserver* observer) {
	mObservers.erase(std::remove(mObservers.begin(), mObservers.end(), observer), mObservers.end());
}

void WindowInputDispatcher::onPointerSample(const PointerSample& sample) {
	if (mEventHandler) {
		InputEvent event;
		event.type = InputEvent::Type::POINTER_SAMPLE;
		event.sample = sample;
		mEventHandler->postEvent(InputID::MOUSE1, event);
	}
	for (WindowInputObserver* observer : mObservers)
		observer->onPointerSample(sample);
}

void WindowInputDispatcher::onPointerMoved(Vec2F contentPosition) {
	if (mEventHandler) mEventHandler->postEvent(InputID::MOUSE1, { InputEvent::Type::MOUSE_POS, {}, contentPosition });
	for (WindowInputObserver* observer : mObservers)
		observer->onPointerMoved(contentPosition);
}

void WindowInputDispatcher::onPointerLeft() {
	releaseHeldButtons();

	const Vec2F outside = { kPointerOutsideCoordinate, kPointerOutsideCoordinate };
	if (mEventHandler) mEventHandler->postEvent(InputID::MOUSE1, { InputEvent::Type::MOUSE_POS, {}, outside });
	for (WindowInputObserver* observer : mObservers)
		observer->onPointerLeft();
}

void WindowInputDispatcher::onPointerButton(InputID button, bool pressed) {
	if (pressed) mButtonsDown.insert(button);
	else if (!mButtonsDown.erase(button)) return;

	postButton(button, pressed);
	for (WindowInputObserver* observer : mObservers)
		observer->onPointerButton(button, pressed);
}

void WindowInputDispatcher::onScrolled(Vec2F delta) {
	if (mEventHandler) mEventHandler->postEvent(InputID::SCROLL, { InputEvent::Type::SCROLL, {}, {}, delta });
	for (WindowInputObserver* observer : mObservers)
		observer->onScrolled(delta);
}

void WindowInputDispatcher::onKey(InputID key, bool pressed) {
	if (pressed) mKeysDown.insert(key);
	else mKeysDown.erase(key);

	postButton(key, pressed);
	for (WindowInputObserver* observer : mObservers)
		observer->onKey(key, pressed);
}

void WindowInputDispatcher::onKeyRepeat(InputID key) {
	for (WindowInputObserver* observer : mObservers)
		observer->onKeyRepeat(key);
}

void WindowInputDispatcher::onText(const std::string& utf8) {
	for (WindowInputObserver* observer : mObservers)
		observer->onText(utf8);
}

void WindowInputDispatcher::onKeyboardFocus(bool focused) {
	if (!focused) {
		const std::set<InputID> released = mKeysDown;
		for (InputID key : released)
			onKey(key, false);
	}

	for (WindowInputObserver* observer : mObservers)
		observer->onKeyboardFocus(focused);
}

void WindowInputDispatcher::onGesture(const TouchGesture& gesture) {
	for (WindowInputObserver* observer : mObservers)
		observer->onGesture(gesture);
}

void WindowInputDispatcher::releaseHeldButtons() {
	const std::set<InputID> held = mButtonsDown;
	for (InputID button : held)
		onPointerButton(button, false);
}

void WindowInputDispatcher::onResized() {
	if (mEventHandler) mEventHandler->postEvent(InputID::WINDOW_RESIZE, {});
}

void WindowInputDispatcher::postButton(InputID id, bool pressed) {
	if (!mEventHandler) return;

	const auto action = pressed ? InputEvent::ButtonAction::PRESS : InputEvent::ButtonAction::RELEASE;
	mEventHandler->postEvent(id, { InputEvent::Type::BUTTON_ACTION, action, {} });
}
