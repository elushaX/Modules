#include "Widget.hpp"

using namespace tp;

Widget::Widget() {
	mArea.setTargetRect({ 10, 10, 100, 40, });
	mArea.endAnimation();
}

void Widget::setArea(const RectF& area) {
	mArea.setTargetRect(area);
}

void Widget::draw(Canvas& canvas) {
	canvas.rect(mArea.getCurrentRect(), RGBA(1.f), 10);
}

RectF Widget::getArea() const {
	return mArea.getCurrentRect();
}

RectF Widget::getRelativeArea() const {
	return { {}, mArea.getCurrentRect().size };
}

void Widget::setActive(bool active) {
	mIsActive = active;
}

void Widget::addChild(Widget* widget) {
	if (widget->mParent) {
		widget->mParent->removeChild(widget);
	}
	mChildWidgets.pushBack(widget);
	widget->mParent = this;
}

void Widget::removeChild(Widget* widget) {
	auto node = mChildWidgets.find(widget);
	node->data->mParent = nullptr;
	mChildWidgets.removeNode(node);
}

bool Widget::needUpdate() const {
	return !mArea.checkAnimationShouldEnd();
}

void Widget::finishAnimations() {
	mArea.endAnimation();
}