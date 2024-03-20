#pragma once

#include "InputCodes.hpp"
#include "Vec.hpp"
#include "List.hpp"
#include "Map.hpp"

#include <mutex>

namespace tp {

	struct InputEvent {
		enum class Type {
			NONE = 0,
			BUTTON_ACTION,
			MOUSE_DELTA,
			MOUSE_POS,
		} type = Type::NONE;

		enum class ButtonAction {
			NONE = 0,
			PRESS,
			RELEASE,
			REPEAT,
		} buttonAction = ButtonAction::NONE;

		Vec2F mouseEvent = { 0, 0 };
	};

	class InputState {
	public:
		InputState() = default;

		void handleEvent(const InputEvent& event);

		bool isStateChanged();

		// true if event is no longer needed and false if we need to emulate some transitions further
		bool isHandled();

	public:
		// Transitions from state to state is strict and defined by state order in the enum
		// If posted event conflicts with that automata, we need to emulate intermediate states
		enum State {
			NONE = 0,     // Button is inactive
			PRESSED,  // Button is pressed
			HOLD,     // Button is still pressed
			RELEASED, // Button is released
		};

		State mCurrentState;
		State mPreviousState;
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
		void postEvent(InputID inputID, InputEvent inputEvent);

	public: // User interface
		bool isEvents();
		void processEvent();

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

		InputState mInputStates[(int) InputID::LAST_KEY_CODE]{};
	};

}