
#include "SimpleLayouts.hpp"
#include "Widget.hpp"


using namespace tp;

void OverlayLayout::updateLayout(bool vertical) {
	if (vertical) return;

	if (children().empty()) return;

	for (auto child : children()) {
		child->getLayout()->setArea(getArea().relative());
	}
}

void ToolBarLayout::updateLayout(bool vertical) {
	if (vertical) return;

	if (children().size() != 2) return;

	auto toolbar = children().back();
	auto content = children().front();

	auto factor = mToolBarHeight / getArea().w;

	auto area = getArea().relative();

	toolbar->getLayout()->setArea(area.splitByFactorVT(factor));
	content->getLayout()->setArea(area);
}