
#include "BasicLayout.hpp"
#include "Widget.hpp"

#include <algorithm>

using namespace tp;


void BasicLayout::pickRect(bool vertical) {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	children.shrinkFromCenter(-mLayoutMargin, true);

	RectF newArea;
	if (vertical) {
		auto rangeY = pickRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);
		newArea = RectF(current.getRangeX(), rangeY);
	} else {
		auto rangeX = pickRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
		newArea = RectF(rangeX, current.getRangeY());
	}

	setArea(newArea);
}

void BasicLayout::arrangeChildren(bool vertical) {
	if (children().empty()) return;

	if (vertical) {
		switch (mLayoutPolicy) {
			case LayoutPolicy::Vertical: adjustLayout(true); break;
			case LayoutPolicy::Passive:
			case LayoutPolicy::Horizontal: break;
		}
	} else {
		switch (mLayoutPolicy) {
			case LayoutPolicy::Horizontal: adjustLayout(false); break;
			case LayoutPolicy::Passive:
			case LayoutPolicy::Vertical: break;
		}
	}
}

halnf BasicLayout::shrinkLayoutSize(WidgetLayout* widget, halnf diff, bool vertical) {
	auto area = widget->getArea();
	auto prevSize = area.size[vertical];
	area.size[vertical] += diff;
	widget->setArea(area);
	//widget->clampMinMaxSize();
	return widget->getArea().size[vertical] - prevSize;
}

bool BasicLayout::canChange(Widget* widget, bool vertical) {
	return widget->getLayout()->getSizePolicy()[vertical] == SizePolicy::Expanding;
}

void BasicLayout::adjustLayout(bool vertical) {

	std::vector<std::pair<WidgetLayout*, bool>> contributors;
	halnf contentSize = 0;

	// find contributors
	for (Widget* child : children()) {
		if (canChange(child, vertical)) {
			contributors.emplace_back( lay(child), true );
		}
	}

	// let each child to chose desired rect
	for (Widget* child : children()) {
		if (canChange(child, vertical)) {
			lay(child)->pickMinimalRect(vertical);
		} else {
			lay(child)->pickRect(vertical);
		}
	}

	// calculate content size
	for (Widget* child : children()) {
		contentSize += lay(child)->getArea().size[vertical];
	}

	halnf availableSize = getArea().size[vertical] - (mLayoutGap * ((halnf) children().size() - 1) + mLayoutMargin * 2);

	if (availableSize < 0) return;

	halnf totalQuota = availableSize - contentSize;

	// expand or contract as much as possible
	while (!contributors.empty() && (totalQuota != 0)) {
		halnf quotaPerContributor = totalQuota / (halnf) contributors.size();

		for (auto& contributor : contributors) {
			halnf contribution = shrinkLayoutSize(contributor.first, quotaPerContributor, vertical);
			if (contribution == 0) {
				contributor.second = false;
			}
			totalQuota -= contribution;
		}

		// remove useless contributors
		contributors.erase(
			std::remove_if(contributors.begin(), contributors.end(), [](auto node) { return !node.second; }),
			contributors.end()
		);
	}

	// set opposite direction size
	for (auto child : children()) {
		RectF area = lay(child)->getArea();
		area.size[!vertical] = getArea().size[!vertical] - mLayoutMargin * 2;
		child->setAreaCache(area);
	}

	// set pos
	halnf iterPos = mLayoutMargin;
	for (auto child : children()) {
		RectF area = lay(child)->getArea();

		area.pos[vertical] = iterPos;
		area.pos[!vertical] = mLayoutMargin;

		iterPos += area.size[vertical] + mLayoutGap;
		lay(child)->setArea(area);
	}
}

RectF BasicLayout::availableChildArea() const {
	auto out = WidgetLayout::availableChildArea();
	return out.shrinkFromCenter(mLayoutMargin, true);
}

RectF BasicLayout::minContentArea() const {
	auto out = WidgetLayout::getChildrenEnclosure();
	out.shrinkFromCenter(-mLayoutMargin, true);
	return out;
}