#pragma once

#include "Layout.hpp"

namespace tp {

	class OverlayLayout : public WidgetLayout {
		friend class DebugManager;

	public:
		explicit OverlayLayout(Widget* widget) : WidgetLayout(widget) {}

		void updateLayout(bool vertical) override;

		void pickRect(bool vertical) override {}
		void clampRect() override {}
	};
}