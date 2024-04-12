#pragma once

#include "EventHandler.hpp"
#include "WidgetManager.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Widget {
	public:
		Widget() { this->mArea = { 0, 0, 100, 100 }; }

		virtual void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) {
			mVisible = areaParent.isOverlap(aArea);
			if (!mVisible) {
				return;
			}

			this->mArea = aArea;
			this->mAreaParent = areaParent;

			procBody(events);
		}

		virtual void draw(Canvas& canvas) {
			if (!mVisible) {
				return;
			}

			drawBody(canvas);
		}

		virtual void procBody(const Events& events) {}
		virtual void drawBody(Canvas& canvas) {}

		virtual void updateConfigCache(WidgetManager& wm) {}

	public:
		RectF mArea;
		RectF mAreaParent;
		bool mVisible = false;
	};
}