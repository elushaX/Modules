
#include "OverlayLayout.hpp"
#include "Widget.hpp"


using namespace tp;

void OverlayLayout::updateLayout(bool vertical) {
	if (vertical) return;

	if (children().empty()) return;

	for (auto child : children()) {
		child->getLayout()->setArea(getArea().relative());
	}

	/*
if (children().empty()) return;

auto first = children().front();

first->getLayout()->setArea(getArea().relative());

for (auto child : children()) {
	if (child == first) continue;
	child->getLayout()->setArea(getArea().relative());
}
	 */
}