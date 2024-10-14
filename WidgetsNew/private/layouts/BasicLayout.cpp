
#include "BasicLayout.hpp"
#include "Widget.hpp"

#include <algorithm>

using namespace tp;


void BasicLayout::pickRect() {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = pickRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = pickRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	auto newArea = RectF(rangeX, rangeY);

	setArea(newArea);
}

void BasicLayout::clampRect() {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = clampRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = clampRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	setArea(RectF(rangeX, rangeY));
}

void BasicLayout::adjustChildrenRect()  {
	if (children().empty()) return;

	switch (mLayoutPolicy) {
		case LayoutPolicy::Passive: break;
		case LayoutPolicy::Vertical: adjustLayout(true); break;
		case LayoutPolicy::Horizontal: adjustLayout(false); break;
	}
}


halnf BasicLayout::changeChildSize(tp::Widget* widget, halnf diff, bool vertical) {
	auto prevSize = widget->getLayout()->getArea().size[vertical];
	{
		auto area = widget->getLayout()->getArea();
		area.size[vertical] += diff;
		widget->setAreaCache(area);

		widget->getLayout()->clampMinMaxSize();
	}
	auto newSize = widget->getLayout()->getArea().size[vertical];
	return newSize - prevSize;
}

void BasicLayout::adjustLayout(bool vertical) {
	std::vector<std::pair<Widget*, bool>> contributors;
	Vec2F contentSize = { 0, 0 };
	Vec2F availableSize = getArea().size;

	for (auto child : children()) {
		if (child->getLayout()->getSizePolicy()[!vertical] != SizePolicy::Minimal) {
			child->getLayout()->pickRect();
		}

		if (child->getLayout()->getSizePolicy()[vertical] == SizePolicy::Expanding) {
			contributors.emplace_back( child, true );

			auto area = child->getLayout()->getArea();
			area.size[vertical] = 0;
			child->setAreaCache(area);
			child->getLayout()->clampRect();
		}

		contentSize += child->getLayout()->getArea().size;
	}

	availableSize -= mLayoutGap * ((halnf) children().size() - 1) + mLayoutMargin * 2;

	auto diff = availableSize - contentSize;

	// expand or contract as much as possible
	while (!contributors.empty() && (diff[vertical] != 0)) {
		auto quota = diff / (halnf) contributors.size();

		for (auto& contributor : contributors) {
			if (!contributor.second) continue;

			// contributor.first->endAnimations();
			auto contribution = changeChildSize(contributor.first, quota[vertical], vertical);

			if (contribution == 0) {
				contributor.second = false;
			}

			diff[vertical] -= contribution;
		}

		contributors.erase(
			std::remove_if(contributors.begin(), contributors.end(), [](auto node) { return !node.second; }),
			contributors.end()
		);
	}

	// arrange
	halnf iterPos = mLayoutMargin;
	for (auto child : children()) {
		auto area = child->getLayout()->getArea();
		area.pos[vertical] = iterPos;
		iterPos += area.size[vertical] + mLayoutGap;
		child->setAreaCache(area);


		// child->updateAnimations();
		// child->triggerWidgetUpdate("layout changed");
	}
}

RectF BasicLayout::getAvailableChildArea() const {
	auto out = WidgetLayout::getAvailableChildArea();
	return out.scaleFromCenter(mLayoutMargin, true);
}