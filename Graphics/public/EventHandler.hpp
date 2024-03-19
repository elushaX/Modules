#pragma once

#include "InputCodes.hpp"
#include "Vec.hpp"
#include "List.hpp"
#include "Map.hpp"

#include <mutex>

namespace tp {

	struct InputEvent {
		enum class Type {
			NONE,
			BUTTON_ACTION,
			MOUSE_DELTA,
			MOUSE_POS,
		} type = Type::NONE;

		enum class ButtonAction {
			NONE,
			PRESS,
			RELEASE,
			REPEAT,
		} buttonAction = ButtonAction::NONE;

		Vec2F mouseEvent = { 0, 0 };
	};

	class InputState {
		// Transitions from state to state is strict and defined by state order in the enum
		// If posted event conflicts with that automata, we need to emulate intermediate states
		enum State {
			NONE, // Button is inactive
			PRESSED, // Button is pressed
			HOLD, // Button is still pressed
			RELEASED, // Button is released
		};

		State mCurrentState;
		State mPreviousState;

	public:
		InputState() = default;

		void handleEvent(const InputEvent& event);

		bool isStateChanged();

		// true if event is no longer needed and false if we need to emulate some transitions further
		bool isHandled();
	};

	// Assumes that user and event poster use different threads
	// User can add own states for each input and decide how this state changes on specific events
	// Event posters has no access to any custom state and only report any changes in any way
	class EventHandler {
	public:
		EventHandler();
		~EventHandler();

	public: // Event Poster Interface

		// Record event
		void postEvent(InputID inputID, InputEvent inputEvent) { 
			mMutex.lock();
			mEventQueue.pushBack({ inputID, inputEvent });
			mMutex.unlock();
		}

	public: // User interface
		bool isEvents() { 
			mMutex.lock();
			auto res = mEventQueue.length();
			mMutex.unlock();
			return res;
		}

		void processEvent() { 
			mMutex.lock();
			mEventQueue.popFront();
			mMutex.unlock();
		}

		const Vec2F& getPointer() const;

		bool isPressed(InputID id) const;
		bool isReleased(InputID id) const;
		bool isDown(InputID id) const;

		halnf getScrollY() const;

	private:
		std::mutex mMutex = {};

		// Store thread protected queue of posted events
		List<std::pair<InputID, InputEvent>> mEventQueue;

		// input states
		Vec2F mPointer;
		halnf pressure = 0;

		InputState mInputStates[(int) InputID::LAST_KEY_CODE];
	};

}