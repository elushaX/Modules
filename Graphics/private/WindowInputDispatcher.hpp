#pragma once

#include "EventHandler.hpp"
#include "WindowInput.hpp"

#include <set>
#include <vector>

namespace tp {

	class WindowInputDispatcher : public WindowInputObserver {
	public:
		static constexpr halnf kPointerOutsideCoordinate = -1.0e5f;

		void setEventHandler(EventHandler* eventHandler) { mEventHandler = eventHandler; }
		[[nodiscard]] EventHandler* eventHandler() const { return mEventHandler; }

		void addObserver(WindowInputObserver* observer);
		void removeObserver(WindowInputObserver* observer);

		void onPointerSample(const PointerSample& sample) override;
		void onPointerMoved(Vec2F contentPosition) override;
		void onPointerLeft() override;
		void onPointerButton(InputID button, bool pressed) override;
		void onScrolled(Vec2F delta) override;
		void onKey(InputID key, bool pressed) override;
		void onKeyRepeat(InputID key) override;
		void onText(const std::string& utf8) override;
		void onKeyboardFocus(bool focused) override;
		void onGesture(const TouchGesture& gesture) override;

		void onResized();

	private:
		void postButton(InputID id, bool pressed);
		void releaseHeldButtons();

		EventHandler* mEventHandler = nullptr;
		std::vector<WindowInputObserver*> mObservers;
		std::set<InputID> mKeysDown;
		std::set<InputID> mButtonsDown;
	};
}
