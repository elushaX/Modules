#include "FloatingLayout.hpp"
#include "Widget.hpp"

using namespace tp;

void FloatingLayout::pickRect(bool vertical) {
	if (mIsFloating) {
		auto area = getArea();

		if (mIsResizing) {
			mPointerCurrent.clamp(mMinSize, mMaxSize);

			area.size[vertical] = (mPointerCurrent + mHandleSize / 2.f)[vertical];

			for (auto child : children()) {
				child->triggerWidgetUpdate("floating menu resized");
			}

		} else if (mIsFloating) {
			area.pos[vertical] += (mPointerCurrent - mPointerStart)[vertical];
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