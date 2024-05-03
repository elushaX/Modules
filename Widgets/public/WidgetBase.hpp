#pragma once

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
		}

		virtual void draw(Canvas& canvas) {
			if (!mVisible) {
				return;
			}
		}

		virtual void updateConfigCache(const WidgetManager& wm) = 0;

	public:
		std::string mId;
		RectF mArea;
		bool mVisible = false;
	};
}