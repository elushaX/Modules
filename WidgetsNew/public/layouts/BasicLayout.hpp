#pragma once

#include "Layout.hpp"

namespace tp {

	class BasicLayout : public WidgetLayout {
	public:
		explicit BasicLayout(Widget* widget) : WidgetLayout(widget) {}

		void pickRect() override;
		void clampRect() override;
		void adjustChildrenRect() override;
		[[nodiscard]] RectF getAvailableChildArea() const override;

	private:
		void adjustLayout(bool vertical);
		static halnf changeChildSize(Widget*, halnf diff, bool vertical);

	private:
		LayoutPolicy mLayoutPolicy = LayoutPolicy::Vertical;
		halnf mLayoutGap = 5;
		halnf mLayoutMargin = 10;
	};
}