#include "ScrollableLayout.hpp"

// fixme : this dependency should be removed
#include "ScrollableWidget.hpp"

using namespace tp;

void ScrollableLayout::updateLayout(bool vertical) {
	if (vertical) return;

	// TODO : make better interface to get scroller and content widget
	if (children().size() != 2) return;

	auto scroller = dynamic_cast<ScrollableBarWidget*>(children().front());
	auto content = children().back();

	if (!scroller || !content) return;

	updateWidgetRects(getArea().relative(), content, scroller);
}

void ScrollableLayout::updateWidgetRects(const RectF& area, Widget* content, ScrollableBarWidget* scroller) const {
	bool dir = scroller->getDirection();

	// update content
	// auto contentItems = content->getLayout()->getChildrenEnclosure();
	// content->getLayout()->setArea(contentItems.shrinkFromCenter(-10, true));
	content->getLayout()->pickRect(dir);

	auto sizeFactor = (area.size[dir] / content->getLayout()->getArea().size[dir]);
	auto splitFactor = 1 - ((sizeFactor > 1.f) ?  0 : mScrollerSize / area.size[!dir]);

	auto holderArea = dir ? area.splitByFactorHL(splitFactor) : area.splitByFactorVT(splitFactor);
	auto contentArea = content->getLayout()->getArea();
	auto scrollerArea = dir ? area.splitByFactorHR(splitFactor) : area.splitByFactorVB(splitFactor);

	scroller->updateSizeFactor(sizeFactor);

	contentArea.pos[dir] = -scroller->getPosFactor() * contentArea.size[dir];
	contentArea.size[!dir] = holderArea.size[!dir];
	contentArea.pos[!dir] = 0;

	scroller->getLayout()->setArea(scrollerArea);
	content->getLayout()->setArea(contentArea);
}

RectF ScrollableLayout::getAvailableChildArea() const {
	auto out = getArea();
	// dont constrain on scroll axis
	out.pos = -FLT_MAX / 4;
	out.size = FLT_MAX / 2;
	return out;
}
