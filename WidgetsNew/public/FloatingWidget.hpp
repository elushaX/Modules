#pragma once

#include "Widget.hpp"

namespace tp {
	class FloatingWidget : public Widget {
	public:
		FloatingWidget() = default;

		void process(const EventHandler& events) override;

		void updateArea(RectF& area) const override;

		void draw(Canvas& canvas) override;

	private:
		bool mIsFloating = false;

		Vec2F mPointerStart;
		Vec2F mPointerCurrent;
	};
}