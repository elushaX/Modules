
#include "EventHandler.hpp"

using namespace tp;

EventHandler::EventHandler() = default;
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
	return res;
}

InputState::State transitions[4][4] = {
	{ InputState::State::NONE, InputState::State::PRESSED, InputState::State::PRESSED, InputState::State::NONE },
	{ InputState::State::PRESSED, InputState::State::PRESSED, InputState::State::HOLD, InputState::State::PRESSED },
	{ InputState::State::HOLD, InputState::State::RELEASED, InputState::State::RELEASED, InputState::State::HOLD },
	{ InputState::State::RELEASED, InputState::State::NONE, InputState::State::NONE, InputState::State::RELEASED },
};

bool transitionsReduce[4][4] = {
	{ true, true, false, true },
	{ true, true, false, true },
	{ true, false, false, true },
	{ true, false, true, true },
};

void EventHandler::processEvent() {
	mMutex.lock();
	auto lastEvent = &mEventQueue.last();

	const auto& eventData = lastEvent->second;
	const auto& inputId = lastEvent->first;

	switch (eventData.type) {
		case InputEvent::Type::MOUSE_POS:
			{
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

		default:
			{
				mEventQueue.popFront();
			}
	}

	mPointerPressure = mInputStates[(int) InputID::MOUSE1].mCurrentState != InputState::State::NONE;

	mMutex.unlock();
}

const Vec2F& EventHandler::getPointer() const { return mPointer; }

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

halnf EventHandler::getScrollY() const { return 0; }