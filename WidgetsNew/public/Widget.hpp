#pragma once

#include "SpringAnimations.hpp"

#include "EventHandler.hpp"
#include "Graphics.hpp"

namespace tp {
	class Widget {
		friend class RootWidget;

	public:
		Widget();
		virtual ~Widget() = default;

		virtual void process(const EventHandler& events) {}
		virtual void updateArea(RectF& area) const {}

		virtual void draw(Canvas& canvas);
		virtual void drawOverlay(Canvas& canvas) {}

		virtual bool isPassThroughEvents() const { return false; }

		[[nodiscard]] virtual bool needUpdate() const;

		virtual void finishAnimations();

	public:
		[[nodiscard]] RectF getRelativeArea() const;
		void setActive(bool active);

		void addChild(Widget* widget);

	private:
		[[nodiscard]] RectF getArea() const;
		void setArea(const RectF& area);
		void removeChild(Widget* widget);

	private:
		// relative to the parent
		SpringRect mArea;

		// tree structure of widgets
		List<Widget*> mChildWidgets;

		// if needed updates even though pointer is not in area
		bool mIsActive = false;

		Widget* mParent = nullptr;

		bool mProcessedFlag = false;
	};
}