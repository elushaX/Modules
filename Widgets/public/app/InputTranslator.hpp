#pragma once

#include "EventHandler.hpp"
#include "Event.hpp"
#include "WindowInput.hpp"

#include <set>
#include <vector>

namespace tp::ui {

	struct InputRouting {
		bool pointerBlocked = false;
		bool keyboardBlocked = false;
	};

	class InputTranslator : public WindowInputObserver {
	public:
		[[nodiscard]] std::vector<Event> translate(const EventHandler& input, const InputRouting& routing);

		void onKey(InputID key, bool pressed) override;
		void onKeyRepeat(InputID key) override;
		void onText(const std::string& utf8) override;
		void onKeyboardFocus(bool focused) override;
		void onGesture(const TouchGesture& gesture) override;

		[[nodiscard]] const Modifiers& modifiers() const { return mModifiers; }

	private:
		void translatePointer(const EventHandler& input, std::vector<Event>& events);
		void translateButtons(const EventHandler& input, std::vector<Event>& events, bool pressesBlocked);
		void translateWheel(const EventHandler& input, std::vector<Event>& events);
		void translateKeys(std::vector<Event>& events, bool pressesBlocked);
		void translateGestures(std::vector<Event>& events, bool pointerBlocked);
		void updateModifiers(InputID key, bool pressed);

		struct PendingKey {
			Event event;
			bool press;
		};

		Vec2F mLastPointer = { -1.0e6f, -1.0e6f };
		bool mPointerKnown = false;
		PointerSample mLastSample;
		bool mButtonDelivered[3] = {};
		std::vector<PendingKey> mPendingKeys;
		std::vector<TouchGesture> mPendingGestures;
		std::set<InputID> mModifiersDown;
		std::set<InputID> mDeliveredKeys;
		Modifiers mModifiers;
	};
}
