#include "Widget.hpp"

using namespace tp;

WidgetManagerInterface* Widget::getRoot() {
	Widget* iter = mParent;
	while (iter && iter->mParent) {
		iter = iter->mParent;
	}
	return dynamic_cast<WidgetManagerInterface*>(iter);
}

void Widget::triggerWidgetUpdate() {
	if (auto root = getRoot()) {
		root->updateWidget(this);
	}
}

Widget::Widget() {
	mArea.setTargetRect({ 10, 10, 100, 40, });
	mArea.endAnimation();
}

void Widget::setArea(const RectF& area) {
	mArea.setTargetRect(area);

	if (!mArea.shouldEndTransition()) {
		triggerWidgetUpdate();
	}
}

RectF Widget::getArea() const {
	return mArea.getCurrentRect();
}

RectF Widget::getRelativeArea() const {
	return { {}, mArea.getCurrentRect().size };
}

void Widget::endAnimations() {
	mArea.endAnimation();
}

bool Widget::processesEvents() const {
	return true;
}

void Widget::updateAnimations() {
	mArea.updateCurrentRect();
}

bool Widget::needsNextFrame() const {
	return !mArea.shouldEndTransition();
}

void Widget::adjustRect() {
	if (mChildren.empty()) return;

	auto area = (*mChildren.begin())->getArea();
	for (auto widget : mChildren) {
		area.expand(widget->getArea());
	}
}

void Widget::bringToFront() {
	if (!mParent) return;
	auto& order = mParent->mDepthOrder;
	auto node = order.find(this);
	DEBUG_ASSERT(node);
	order.detach(node);
	order.pushFront(node);
}

void Widget::bringToBack() {
	if (!mParent) return;
	auto& order = mParent->mDepthOrder;
	auto node = order.find(this);
	DEBUG_ASSERT(node);
	order.detach(node);
	order.pushBack(node);
}

void Widget::adjustChildrenRect()  {
	auto area = RectF({}, getArea().size);
	for (auto widget : mChildren) {
		widget->setArea(area);
	}
}

void Widget::mouseEnter() {
	mInFocus = true;
}

void Widget::mouseLeave() {
	mInFocus = false;
}

bool Widget::propagateEventsToChildren() const {
	return true;
}

void Widget::addChild(Widget* child) {
	mChildren.push_back(child);
	mDepthOrder.pushBack(child);

	child->mParent = this;

	triggerWidgetUpdate();
	child->triggerWidgetUpdate();
}
