#include "WidgetBase.hpp"

using namespace tp;

Widget::Widget() { mArea = { 0, 0, 100, 100 }; }

Widget::~Widget() = default;

void Widget::procWrapper(const Events& events, const RectF& parentArea) {
	if (!mEnable) return;

	checkVisibility(events, parentArea);

	if (!mVisible) return;

	checkFocus(events);

	checkClicked(events);

	if (mHandlesEvents) {

		eventProcess(events);

		for (auto child : mChildWidgets) {
			child->procWrapper(events, mVisibleArea);
		}
	}
}

void Widget::drawWrapper(Canvas& canvas) {
	if (!mEnable || !mVisible) return;

	eventDraw(canvas);

	// draw child widgets
	canvas.pushClamp(this->mArea);
	for (auto child = mChildWidgets.lastNode(); child; child = child->prev) {
		child->data->drawWrapper(canvas);
	}
	canvas.popClamp();

	eventDrawOver(canvas);
}

void Widget::updateConfigWrapper(WidgetManager& wm) {
	wm.setActiveId("Global");

	eventUpdateConfiguration(wm);

	for (auto child : mChildWidgets) {
		child->updateConfigWrapper(wm);
	}
}

void Widget::eventProcess(const Events& events) {}
void Widget::eventDraw(Canvas& canvas) {}
void Widget::eventDrawOver(Canvas& canvas) {}
void Widget::eventUpdateConfiguration(WidgetManager& wm) {}

void Widget::eventVisible(const Events& events) {}
void Widget::eventNotVisible(const Events& events) {}

void Widget::eventFocusEnter(const Events& events) {}
void Widget::eventFocusLeave(const Events& events) {}

void Widget::eventPressed(const Events& events) {}
void Widget::eventReleased(const Events& events) {}

void Widget::checkVisibility(const Events& events, const RectF& parentArea) {
	const bool currentVisibility = parentArea.isOverlap(getArea());

	parentArea.calcIntersection(mArea, mVisibleArea);

	if (currentVisibility != mVisible) {
		if (currentVisibility) eventVisible(events);
		else eventNotVisible(events);
	}

	mVisible = currentVisibility;

	if (!mVisible) {
		mHolding = false;
		mPressed = false;
		mReleased = false;
	}
}

void Widget::checkFocus(const Events& events) {
	const bool currentFocus = mVisibleArea.isInside(events.getPointerPrev());

	if (currentFocus != mInFocus) {
		if (currentFocus) eventFocusEnter(events);
		else eventFocusLeave(events);
	}

	mInFocus = currentFocus;

	if (!mInFocus) {
		mHolding = false;
		mPressed = false;
		mReleased = false;
	}
}

void Widget::checkClicked(const Events& events) {
	mPressed = false;
	mReleased = false;

	if (!mInFocus) return;

	if (mHolding) {
		if (events.isReleased(InputID::MOUSE1)) {
			eventReleased(events);
			mReleased = true;
			mHolding = false;
		}
	} else {
		if (events.isPressed(InputID::MOUSE1) && mVisibleArea.isInside(events.getPointer())) {
			eventPressed(events);
			mHolding = true;
			mPressed = true;
		}
	}
}

void Widget::setEnable(bool enable) { mEnable = enable; }
void Widget::setVisible(bool visible) { mVisible = visible; }
void Widget::setArea(const RectF& area) { mArea = area; }

const RectF& Widget::getArea() const { return mArea; }
bool Widget::isFocus() const { return mInFocus; }
bool Widget::isPressed() const { return mPressed; }
bool Widget::isReleased() const { return mReleased; }
bool Widget::isHolding() const { return mHolding; }

void Widget::clearEvents() {
	mReleased = false;
	mPressed = false;
	mHolding = false;
}
