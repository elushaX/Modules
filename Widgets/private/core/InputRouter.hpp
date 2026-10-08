#pragma once

#include "Diagnostics.hpp"
#include "Event.hpp"
#include "WindowTypes.hpp"

#include <set>
#include <vector>

namespace tp::ui {

	class Ui;
	class Widget;
	class PopupStack;
	class TooltipController;

	class InputRouter {
	public:
		InputRouter(Ui& ui, PopupStack& popups, TooltipController& tooltips, Diagnostics& diagnostics);

		void dispatch(Event event);
		void advanceClock(halnf deltaMs);
		[[nodiscard]] bool isWaitingForLongPress() const { return mLongPressArmed; }
		void refreshHover();
		void refreshCursor() { applyCursor(); }
		void forget(const Widget& widget);

		void capture(Widget& widget);
		void release(const Widget& widget);
		[[nodiscard]] Widget* captured() const { return mCapture; }

		void setKeyboardFocus(Widget* widget);
		[[nodiscard]] Widget* keyboardFocus() const { return mKeyboardFocus; }
		[[nodiscard]] bool isKeyboardNavigating() const { return mKeyboardNavigating; }

		void setActiveWidget(Widget* widget);
		[[nodiscard]] Widget* activeWidget() const { return mActive; }

		[[nodiscard]] Widget* hovered() const { return mHovered; }
		[[nodiscard]] Widget* pressed() const { return mPressed; }
		[[nodiscard]] Vec2F pointer() const { return mPointer; }
		[[nodiscard]] bool isPrimaryDown() const { return mButtonsDown[(int) PointerButton::Primary]; }
		[[nodiscard]] bool isKeyDown(InputID key) const { return mKeysDown.count(key) > 0; }

		[[nodiscard]] Widget* widgetAt(Vec2F windowPoint) const;

	private:
		void handlePointerMove(Event& event);
		void handlePointerDown(Event& event);
		void handlePointerUp(Event& event);
		void handleWheel(Event& event);
		void handleKey(Event& event);
		void handleGesture(Event& event);

		[[nodiscard]] int countClick(const Event& event);
		void armLongPress(const Event& event);
		void fireLongPress();
		void applyCursor();

		void dismissPopupsOutside(Widget* target);
		void updateHover(Widget* target);
		void updatePressed();
		[[nodiscard]] Widget* pressedWidget() const;
		static void updatePathFlags(
			const std::vector<Widget*>& previousPath, const std::vector<Widget*>& nextPath, void (*setFlag)(Widget&, bool)
		);
		[[nodiscard]] std::vector<Widget*> ownershipPathTo(Widget* widget) const;
		void bubble(Event& event, Widget* target);
		void deliverTo(Event& event, Widget& receiver, Widget* target);

		[[nodiscard]] Widget* pointerTarget() const;
		[[nodiscard]] bool isBlockedByModal(const Widget* target) const;
		[[nodiscard]] static Widget* hitTestSubtree(Widget& widget, Vec2F windowPoint);
		[[nodiscard]] static std::vector<Widget*> pathTo(Widget* widget);

		Ui& mUi;
		PopupStack& mPopups;
		TooltipController& mTooltips;
		Diagnostics& mDiagnostics;

		Vec2F mPointer = { -1.0e6f, -1.0e6f };
		bool mButtonsDown[3] = { false, false, false };
		std::set<InputID> mKeysDown;

		Widget* mHovered = nullptr;
		Widget* mGestureTarget = nullptr;
		Widget* mPressed = nullptr;
		Widget* mCapture = nullptr;
		Widget* mKeyboardFocus = nullptr;
		bool mKeyboardNavigating = false;
		std::vector<Widget*> mPressedPath;

		halnf mClockMs = 0;
		halnf mLastPressMs = -1.0e9f;
		Vec2F mLastPressPosition = { -1.0e6f, -1.0e6f };
		PointerButton mLastPressButton = PointerButton::Primary;
		int mClickCount = 0;
		bool mLongPressArmed = false;
		bool mLongPressConsumed = false;
		halnf mLongPressDueMs = 0;
		Vec2F mLongPressOrigin = { 0, 0 };
		Modifiers mLongPressModifiers;
		PointerSample mLongPressSample;
		CursorShape mAppliedCursor = CursorShape::Default;
		Widget* mActive = nullptr;
		std::vector<Widget*> mActivePath;
	};
}
