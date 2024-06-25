#pragma once

#include "Graphics.hpp"

#include "EventHandler.hpp"
#include "WidgetManager.hpp"
#include "List.hpp"

namespace tp {

	using Events = EventHandler;

	class Widget {
	public:
		Widget();
		virtual ~Widget();

		void procWrapper(const Events& events, const RectF& parentArea);
		void drawWrapper(Canvas& canvas);
		void updateConfigWrapper(WidgetManager& wm);

		virtual void eventProcess(const Events& events);
		virtual void eventDraw(Canvas& canvas);
		virtual void eventUpdateConfiguration(WidgetManager& wm);

		virtual void eventVisible(const Events& events);
		virtual void eventNotVisible(const Events& events);

		virtual void eventFocusEnter(const Events& events);
		virtual void eventFocusLeave(const Events& events);

		virtual void eventPressed(const Events& events);
		virtual void eventReleased(const Events& events);

	public:
		void setEnable(bool enable);
		void setVisible(bool visible);
		void setArea(const RectF& area);

		[[nodiscard]] const RectF& getArea() const;
		[[nodiscard]] bool isFocus() const;
		[[nodiscard]] bool isPressed() const;
		[[nodiscard]] bool isReleased() const;
		[[nodiscard]] bool isHolding() const;

		void clearEvents();

	private:
		void checkVisibility(const Events& events, const RectF& parentArea);
		void checkFocus(const Events& events);
		void checkClicked(const Events& events);

	public:
		RectF mArea;

		List<Widget*> mChildWidgets;

		bool mVisible = false;
		bool mEnable = true;
		bool mHandlesEvents = true;
		bool mInFocus = false;

		bool mHolding = false;
		bool mPressed = false;
		bool mReleased = false;


		// docking
		bool mIsDocked = false;
	};
}