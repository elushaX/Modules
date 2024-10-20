#pragma once

#include "Layout.hpp"

namespace tp {

	class BasicLayout : public WidgetLayout {
		friend class DebugManager;

	public:
		explicit BasicLayout(Widget* widget) : WidgetLayout(widget) {}

		void arrangeChildren(bool vertical) override;

		void pickRect(bool vertical) override;

		[[nodiscard]] RectF availableChildArea() const override;
		[[nodiscard]] RectF minContentArea() const override;

		void setLayoutPolicy(LayoutPolicy layout) { mLayoutPolicy = layout; }

	private:
		static bool canChange(Widget*, bool);

	private:
		void adjustLayout(bool vertical);
		static halnf shrinkLayoutSize(WidgetLayout* widget, halnf diff, bool vertical);

	private:
		LayoutPolicy mLayoutPolicy = LayoutPolicy::Vertical;
		halnf mLayoutGap = 5;
		halnf mLayoutMargin = 9;
	};
}