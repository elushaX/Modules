#include "LayoutManager.hpp"
#include "DebugManager.hpp"

#include "DockLayout.hpp"
#include "ScrollableLayout.hpp"

#include <algorithm>

using namespace tp;

void LayoutManager::adjust(Widget* root) {
	for (auto i : IterRange(2)) {
		mVertical = bool(i);

		mDepGraph.clear();
		mLayOrder.clear();
		mRoots.clear();

		findDependencies(root);

		// TODO : implement better topological-sort
		for (auto iterRoot : mRoots) {
			topologicalSort(iterRoot);
		}

		adjustLayouts();
	}
}

void LayoutManager::findDependencies(Widget* root) {
	mDepGraph.insert({ root, {} });

	Widget::dfs(root, [this](Widget* widget) { mDepGraph.insert({ widget, {} }); });

	for (auto& [widget, deps] : mDepGraph) {
		for (auto child : widget->mChildren) {
			mDepGraph.insert({ child, {} });

			if (auto depOrder = getLayoutOrder(widget->getLayout(), child->getLayout())) {
				if (depOrder > 0) {
					deps.depends.push_back(child);
					mDepGraph[child].references++;
				} else {
					mDepGraph[child].depends.push_back(widget);
					mDepGraph[widget].references++;
				}
			}
		}
	}

	// find root
	for (auto& node : mDepGraph) {
		if (node.second.references == 0) {
			mRoots.push_back(node.first);
		}
	}
}

void LayoutManager::topologicalSort(Widget* root, int depth) {
	mDepGraph.at(root).depth = max(mDepGraph.at(root).depth, depth);

	for (auto& dep : mDepGraph.at(root).depends) {
		topologicalSort(dep, depth + 1);
	}
}

void LayoutManager::adjustLayouts() {
	for (auto& iter : mDepGraph) {
		mLayOrder.emplace_back(iter.first, iter.second.depth);
	}

	std::sort(mLayOrder.begin(), mLayOrder.end(), [](auto first, auto second){
		return first.second > second.second;
	});

	for (auto& [iter, _] : mLayOrder) {
		iter->getLayout()->updateLayout(mVertical);
	}
}

static int sizePolicyDep[3][3] = {
	// f s e child
	{ 1, 1, 1 }, // parent fixed
	{ 1, 1, 1 }, // parent shrink
	{ 1, 1, 1 }, // parent expand
};

int LayoutManager::getLayoutOrder(WidgetLayout* parent, WidgetLayout* child) const {
	if (!parent || !child) return 0;

	auto policyParent = parent->getSizePolicy()[mVertical];
	auto policyChild = parent->getSizePolicy()[mVertical];

	if (dynamic_cast<DockLayout*>(parent)) return -1;
	// if (dynamic_cast<ScrollableLayout*>(parent)) return -1;

	return sizePolicyDep[int(policyParent)][int(policyChild)];
}
