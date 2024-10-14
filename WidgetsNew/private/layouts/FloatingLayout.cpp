#include "FloatingLayout.hpp"
#include "Widget.hpp"

using namespace tp;

void FloatingLayout::pickRect() {
	if (mIsFloating) {
		auto area = getArea();

		if (mIsResizing) {
			mPointerCurrent.clamp(mMinSize, mMaxSize);

			area.size = mPointerCurrent + mHandleSize / 2.f;

			for (auto child : children()) {
				child->triggerWidgetUpdate("floating menu resized");
			}

		} else if (mIsFloating) {
			area.pos += mPointerCurrent - mPointerStart;
		}

		setArea(area);
	}

	clampMinMaxSize();
}

RectF FloatingLayout::resizeHandleRect() {
	auto area = getAnimatedArea().relative();
	area.pos = area.p3() - mHandleSize;
	area.size = mHandleSize;
	area.shrink(mHandlePadding);
	return area;
}

void FloatingLayout::startAction(const Vec2F& pointer) {
	mPointerStart = pointer;

	mIsFloating = true;
	if (resizeHandleRect().isInside(mPointerStart)) {
		mIsResizing = true;
	}
}

void FloatingLayout::updateAction(const Vec2F& pointer) {
	mPointerCurrent = pointer;
}

void FloatingLayout::endAction() {
	mIsResizing = mIsFloating = false;
}