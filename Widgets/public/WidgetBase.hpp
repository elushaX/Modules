#pragma once

#include "EventHandler.hpp"
#include "WidgetManager.hpp"
#include "List.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Widget {
	public:
		Widget() { mArea = { 0, 0, 100, 100 }; }

		virtual ~Widget() = default;

		virtual void procWrapper(const Events& events, const RectF& parentArea) {
			if (!mEnable) return;

			checkVisibility(events, parentArea);

			if (!mVisible) return;

			checkFocus(events);

			checkClicked(events);

			eventProcess(events);

			for (auto child : mChildWidgets) {
				child->procWrapper(events, getArea());
			}
		}

		virtual void drawWrapper(Canvas& canvas) {
			if (!mEnable || !mVisible) return;

			eventDraw(canvas);

			// draw child widgets
			canvas.pushClamp(this->mArea);
			for (auto child : mChildWidgets) {
				child->drawWrapper(canvas);
			}
			canvas.popClamp();
		}

		virtual void updateConfigWrapper(WidgetManager& wm) {
			wm.setActiveId("Global");

			eventUpdateConfiguration(wm);

			for (auto child : mChildWidgets) {
				child->updateConfigWrapper(wm);
			}
		}

		virtual void eventProcess(const Events& events) {}
		virtual void eventDraw(Canvas& canvas) {}
		virtual void eventUpdateConfiguration(WidgetManager& wm) {}

		virtual void eventVisible(const Events& events) {}
		virtual void eventNotVisible(const Events& events) {}

		virtual void eventFocusEnter(const Events& events) {}
		virtual void eventFocusLeave(const Events& events) {}

		virtual void eventPressed(const Events& events) {}
		virtual void eventReleased(const Events& events) {}

	public:
		void checkVisibility(const Events& events, const RectF& parentArea) {
			const bool currentVisibility = parentArea.isOverlap(getArea());

			if (currentVisibility != mVisible) {
				if (currentVisibility) eventVisible(events);
				else eventNotVisible(events);
			}

			mVisible = currentVisibility;

			if (!mVisible) {
				mHolding = false;
				mPressed = false;
				mReleased = false;
			}
		}

		void checkFocus(const Events& events) {
			const bool currentFocus = getArea().isInside(events.getPointer());

			if (currentFocus != mInFocus) {
				if (currentFocus) eventFocusEnter(events);
				else eventFocusLeave(events);
			}

			mInFocus = currentFocus;

			if (!mInFocus) {
				mHolding = false;
				mPressed = false;
				mReleased = false;
			}
		}

		void checkClicked(const Events& events) {
			mPressed = false;
			mReleased = false;

			if (!mInFocus) return;

			if (mHolding) {
				if (events.isReleased(InputID::MOUSE1)) {
					eventReleased(events);
					mReleased = true;
					mHolding = false;
				}
			} else {
				if (events.isPressed(InputID::MOUSE1)) {
					eventPressed(events);
					mHolding = true;
					mPressed = true;
				}
			}
		}

	public:
		void setEnable(bool enable) { mEnable = enable; }
		void setVisible(bool visible) { mVisible = visible; }
		void setArea(const RectF& area) { mArea = area; }

		const RectF& getArea() const { return mArea; }
		bool isFocus() const { return mInFocus; }
		bool isPressed() const { return mPressed; }
		bool isReleased() const { return mReleased; }
		bool isHolding() const { return mHolding; }

		void clearEvents() {
			mReleased = false;
			mPressed = false;
			mHolding = false;
		}

	public:
		RectF mArea;

		List<Widget<Events, Canvas>*> mChildWidgets;

		bool mVisible = false;
		bool mEnable = true;
		bool mInFocus = false;

		bool mHolding = false;
		bool mPressed = false;
		bool mReleased = false;

		// Vec2F mPressedRelativeLocation;
	};
}