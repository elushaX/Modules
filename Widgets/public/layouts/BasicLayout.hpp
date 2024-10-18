#pragma once

#include "Layout.hpp"

namespace tp {

	class BasicLayout : public WidgetLayout {
		friend class DebugManager;

	public:
		explicit BasicLayout(Widget* widget) : WidgetLayout(widget) {}

		void updateLayout(bool vertical) override;

		void pickRect(bool vertical) override;
		void clampRect() override;
		[[nodiscard]] RectF getAvailableChildArea() const override;

		void setLayoutPolicy(LayoutPolicy layout) { mLayoutPolicy = layout; }

	private:
		void adjustLayout(bool vertical);
		static halnf changeChildSize(Widget*, halnf diff, bool vertical);

	private:
		LayoutPolicy mLayoutPolicy = LayoutPolicy::Vertical;
		halnf mLayoutGap = 5;
		halnf mLayoutMargin = 9;
	};
}