#pragma once

#include "Layout.hpp"

namespace tp {
	class LayoutManager {
	public:
		LayoutManager() = default;
		void adjust(Widget* root);
	};
}