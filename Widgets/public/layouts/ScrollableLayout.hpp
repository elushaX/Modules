#pragma once

#include "Layout.hpp"

namespace tp {
	class ScrollableBarWidget;

	class ScrollableLayout : public WidgetLayout {
	public:
		explicit ScrollableLayout(Widget* widget) :
			WidgetLayout(widget) {
			setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		}

		void updateLayout(bool vertical) override;
		[[nodiscard]] RectF getAvailableChildArea() const override;

	private:
		void updateWidgetRects(const RectF& area, Widget* content, ScrollableBarWidget* scroller) const;

	private:
		halnf mScrollerSize = 15;
	};
}