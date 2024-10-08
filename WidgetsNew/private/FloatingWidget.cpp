#include "FloatingWidget.hpp"

using namespace tp;

void FloatingWidget::process(const EventHandler& events) {
	const auto relativePointer = events.getPointer();

	bool inside = getRelativeArea().isInside(relativePointer);

	if (inside && events.isPressed(InputID::MOUSE1)) {
		mPointerStart = relativePointer;
		mIsFloating = true;
	}

	if (mIsFloating && resizeHandleRect().isInside(relativePointer)) {
    mIsResizing = true;
	}

	if (mIsFloating && events.isReleased(InputID::MOUSE1)) {
		mIsFloating = false;
		mIsResizing = false;
	}

	mPointerCurrent = relativePointer;

	// triggerWidgetUpdate();
}

void FloatingWidget::adjustRect() {
	auto area = getArea();

	if (mIsResizing) {
		area.size = mPointerCurrent + mHandleSize / 2.f;
	} else if (mIsFloating) {
		area.pos += mPointerCurrent - mPointerStart;
	}

	setArea(area);
}

void FloatingWidget::draw(Canvas& canvas) {
	canvas.rect(resizeHandleRect(), RGBA(0.7f), 2);
	canvas.rect(getRelativeArea(), RGBA(0.5f), 10);
}

RectF FloatingWidget::resizeHandleRect() {
	auto area = getRelativeArea();
	area.pos = area.p3() - mHandleSize;
	area.size = mHandleSize;
	area.shrink(mHandlePadding);
	return area;
}

bool FloatingWidget::propagateEventsToChildren() const {
	return !mIsFloating;
}