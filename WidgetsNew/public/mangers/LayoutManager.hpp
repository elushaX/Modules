#pragma once

#include "Layout.hpp"
#include "DebugManager.hpp"

#include <map>
#include <vector>

namespace tp {
	class LayoutManager {
		friend DebugManager;

		struct DepNode {
			std::vector<Widget*> depends;
			int references = 0;
			int depth = 0;
		};

	public:
		LayoutManager() = default;

		void adjust(Widget* root);

	private:
		void findDependencies(Widget* root);
		void topologicalSort(Widget* root, int depth = 0);
		void adjustLayouts();

	private:
		int getLayoutOrder(WidgetLayout* parent, WidgetLayout* child) const;

	private:
		std::map<Widget*, DepNode> mDepGraph;
		std::vector<Widget*> mRoots;

		std::vector<std::pair<Widget*, int>> mLayOrder;
		bool mVertical = false;
	};
}