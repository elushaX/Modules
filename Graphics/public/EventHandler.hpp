#pragma once

#include "InputCodes.hpp"
#include "Vec.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Timing.hpp"

#include <mutex>

namespace tp {

	struct InputEvent {
		enum class Type {
			NONE = 0,
			BUTTON_ACTION,
			MOUSE_DELTA,
			MOUSE_POS,
			SCROLL,
		} type = Type::NONE;

		enum class ButtonAction {
			NONE = 0,
			PRESS,
			HOLD,
			RELEASE,
			REPEAT,
		} buttonAction = ButtonAction::NONE;

		Vec2F mouseEvent = { 0, 0 };
		Vec2F scrollDelta = { 0, 0 };
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
			NONE = 0, // Button is inactive
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
		void processAllEvent();

		void setCursorOrigin(const Vec2F& origin);

		[[nodiscard]] Vec2F getPointer() const;
		[[nodiscard]] Vec2F getPointerPrev() const;
		[[nodiscard]] Vec2F getPointerDelta() const;

		[[nodiscard]] bool isPressed(InputID id) const;
		[[nodiscard]] bool isReleased(InputID id) const;
		[[nodiscard]] bool isDown(InputID id) const;
		[[nodiscard]] halnf getScrollY() const;

		[[nodiscard]] halnf getPointerPressure() const;

		void setEnableKeyEvents(bool);

	private:
		void processEventUnguarded();

	private:
		std::mutex mMutex = {};

		// Store thread protected queue of posted events
		List<std::pair<InputID, InputEvent>> mEventQueue;

		// input states
		Vec2F mPointerOrigin = { 0, 0 };

		Vec2F mPointer;
		Vec2F mPointerPrev;
		Vec2F mScrollDelta;

		halnf mPointerPressure = 0;

		InputState mInputStates[(int) InputID::LAST_KEY_CODE]{};

		Timer mTimerEvent = Timer(1000);

		bool mEnableKeyEvents = true;
	};

}