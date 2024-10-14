#pragma once

#include "Widgets.hpp"

namespace tp {
	class WorkspaceWidget : public Widget {
	public:
		WorkspaceWidget();

		void eventProcess(const Events& events) override;

	protected:
		DockWidget mDockSpace;
		FloatingLayoutWidget mFloatingLayer;

		// Parameters
		Vec2F mDefaultFloatSize = { 200, 200 };
	};
}
