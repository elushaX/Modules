
#include "EventHandler.hpp"

using namespace tp;

EventHandler::EventHandler() {
	mTimerEvent.reset();
}

EventHandler::~EventHandler() = default;

void EventHandler::postEvent(InputID inputID, InputEvent inputEvent) {
	mMutex.lock();
	mEventQueue.pushBack({ inputID, inputEvent });
	mMutex.unlock();
}

bool EventHandler::isEvents() {
	mMutex.lock();
	auto res = mEventQueue.length();
	mMutex.unlock();

	if (mTimerEvent.isTimeout()) {
		mTimerEvent.reset();
		res = true;
	}

	return res;
}

InputState::State transitions[4][4] = {
	{ InputState::State::NONE, InputState::State::PRESSED, InputState::State::PRESSED, InputState::State::NONE },
	{ InputState::State::PRESSED, InputState::State::HOLD, InputState::State::HOLD, InputState::State::HOLD },
	{ InputState::State::HOLD, InputState::State::HOLD, InputState::State::HOLD, InputState::State::RELEASED },
	{ InputState::State::NONE, InputState::State::NONE, InputState::State::NONE, InputState::State::NONE },
};

bool transitionsReduce[4][4] = {
	{ true, false, false, true },
	{ true, true, true, false },
	{ true, true, true, false },
	{ true, true, true, true },
};

void EventHandler::processAllEvent() {
	mMutex.lock();
	while (mEventQueue.size()) {
		processEventUnguarded();
	}
	mMutex.unlock();
}

void EventHandler::processEvent() {
	mMutex.lock();

	if (!mEventQueue.size()) {
		mMutex.unlock();
		return;
	}

	processEventUnguarded();

	mMutex.unlock();
}

void EventHandler::processEventUnguarded() {

	auto firstEvent = &mEventQueue.first();

	const auto& eventData = firstEvent->second;
	const auto& inputId = firstEvent->first;

	switch (eventData.type) {
		case InputEvent::Type::MOUSE_POS:
			{
				mPointerPrev = mPointer;
				mPointer = eventData.mouseEvent;
				mEventQueue.popFront();
				break;
			}

		case InputEvent::Type::BUTTON_ACTION:
			{
				auto currentState = (int) mInputStates[(int) inputId].mCurrentState;
				auto reportedEvent = (int) eventData.buttonAction;

				mInputStates[(int) inputId].mCurrentState = transitions[currentState][reportedEvent];

				if (transitionsReduce[currentState][reportedEvent]) {
					mEventQueue.popFront();
				}

				break;
			}

		case InputEvent::Type::SCROLL:
			{
				if (mScrollDelta == Vec2F { 0, 0 }) {
					mScrollDelta = eventData.scrollDelta;
				} else {
					mEventQueue.popFront();
					mScrollDelta = 0;
				}
				break;
			}

		default:
			{
				mEventQueue.popFront();
			}
	}

	mPointerPressure = mInputStates[(int) InputID::MOUSE1].mCurrentState != InputState::State::NONE;
}

void EventHandler::setCursorOrigin(const Vec2F& origin) {
	mPointerOrigin = origin;
}

Vec2F EventHandler::getPointer() const { return mPointer - mPointerOrigin; }

Vec2F EventHandler::getPointerPrev() const { return mPointerPrev - mPointerOrigin; }

bool EventHandler::isPressed(InputID id) const {
	return mInputStates[(int) id].mCurrentState == InputState::State::PRESSED;
}

bool EventHandler::isReleased(InputID id) const {
	return mInputStates[(int) id].mCurrentState == InputState::State::RELEASED;
}

halnf EventHandler::getPointerPressure() const { return mPointerPressure; }

bool EventHandler::isDown(InputID id) const {
	return mInputStates[(int) id].mCurrentState == InputState::State::PRESSED ||
				 mInputStates[(int) id].mCurrentState == InputState::State::HOLD;
}

halnf EventHandler::getScrollY() const { return mScrollDelta.y; }

Vec2F EventHandler::getPointerDelta() const { return mPointer - mPointerPrev; }
