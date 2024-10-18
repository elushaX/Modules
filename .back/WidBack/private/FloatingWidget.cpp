
#include "FloatingWidget.hpp"

using namespace tp;

FloatingWidget::FloatingWidget() {
	this->mArea = { 0, 0, 300, 300 };
	this->mAdjustHeight = false;
}

void FloatingWidget::eventProcess(const Events& events) {
	mActionStartRelativePos = events.getPointerPrev() - this->mArea.pos;

	checkFloating(events);
	if (mResizable) checkResizing(events);

	CollapsableMenu::eventProcess(events);
}

void FloatingWidget::eventDraw(Canvas& canvas) {
	CollapsableMenu::eventDraw(canvas);

	if (!this->getCollapsed() && mResizable) {
		auto rect = getResizeHandle();
		canvas.rect(rect, mResizeHandleColor, 0);
		canvas.circle(rect.pos - this->mBorderSize, rect.w, this->mMenuColor);
	}
}

void FloatingWidget::eventUpdateConfiguration(WidgetManager& wm) {
	CollapsableMenu::eventUpdateConfiguration(wm);

	wm.setActiveId("FloatingWidget");

	mResizeHandleSize = wm.getNumber("ResizeHandleSize", 15);
	mResizeHandleColor = wm.getColor("ResizeHandleColor", RGBA(0.16, 0.16, 0.16, 1.f));
}

void FloatingWidget::checkFloating(const Events& events) {
	mDropped = false;

	if (this->mHeader.isHolding() && events.getPointerDelta().length2() > 4) {
		mFloating = true;
	}

	if (mFloating && this->mHeader.isReleased()) {
		mFloating = false;
		this->mHeader.clearEvents();
		mDropped = true;
	}

	if (mFloating) {
		auto relativePos = events.getPointer() - this->mArea.pos;
		this->mArea.pos += relativePos - mActionStartRelativePos;
	}
}

void FloatingWidget::checkResizing(const Events& events) {
	if (this->getCollapsed()) return;

	if (events.isPressed(InputID::MOUSE1) && getResizeHandle().isInside(events.getPointer())) {
		mResizing = true;
	}

	if (events.isReleased(InputID::MOUSE1)) {
		mResizing = false;
	}

	if (mResizing) {
		auto relativePos = events.getPointer() - this->mArea.pos;
		this->mArea.size += relativePos - mActionStartRelativePos;
	}

	if (!this->mCollapsed) {
		this->mArea.size.clamp(mMinSize, { FLT_MAX, FLT_MAX });
	}
}

RectF FloatingWidget::getResizeHandle() {
	auto size = Vec2F(mResizeHandleSize);
	auto pos = this->mArea.pos + this->mArea.size - size;
	return { pos, size };
}

bool FloatingWidget::isFloating() const { return mFloating; }

void FloatingWidget::stopFloating() {
	mFloating = false;
}
