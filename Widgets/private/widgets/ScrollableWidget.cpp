#include "ScrollableWidget.hpp"

using namespace tp;

void ScrollableBarWidget::process(const EventHandler& events) {
	// all content is visible no need to process anything
	if (mSizeFactor >= 1) {
		mScrolling = false;
		mPosFactor = mSizeFactor / 2.f;
		freeFocus();
		return;
	}

	updateHandleRect();

	auto pointer = events.getPointer();
	auto size = getArea().size;

	mHandleHovered = getHandleRect().isInside(pointer);

	if (getRelativeAreaT().isInside(pointer) && events.isPressed(InputID::MOUSE1)) {
		if (!mHandleHovered) {
			jumpTo((pointer / size)[mVertical]);
			mStartPos = { 0, 0 };
		} else {
			mStartPos = pointer - getHandleRect().center();
		}
		mScrolling = true;
		lockFocus();
	}

	if (mScrolling && events.isReleased(InputID::MOUSE1)) {
		mScrolling = false;
		freeFocus();
	}

	if (mScrolling) {
		updateHandleRect();
		auto dragDelta = (pointer - getHandleRect().center()) - mStartPos;
		moveBy((dragDelta / size)[mVertical]);
	}
}

void ScrollableBarWidget::jumpTo(halnf pos) {
	mPosFactor = pos;
	clamp();
}

void ScrollableBarWidget::moveBy(halnf delta) {
	mPosFactor += delta;
	clamp();
}

void ScrollableBarWidget::updateSizeFactor(halnf factor) {
	mSizeFactor = factor;
	clamp();
}

void ScrollableBarWidget::clamp() {
	mSizeFactor = ::clamp(mSizeFactor, 0.f, 1.f);
	mPosFactor = ::clamp(mPosFactor, mSizeFactor / 2, 1.f - mSizeFactor / 2);
}

void ScrollableBarWidget::draw(Canvas& canvas) {
	updateHandleRect();

	canvas.rect(getArea().relative(), mBGColor, mRounding);

	auto& handleColor = mScrolling ? mHandleSlideColor : (mHandleHovered ? mHandleHoverColor : mHandleColor);
	canvas.rect(getHandleRect(), handleColor, mRounding);
}

void ScrollableBarWidget::updateHandleRect() {
	auto area = getArea().relative();

	mHandleRect = area;
	mHandleRect.size[mVertical] = area.size[mVertical] * mSizeFactor;
	mHandleRect.pos[mVertical] = area.size[mVertical] * mPosFactor;

	mHandleRect.pos[mVertical] -= mHandleRect.size[mVertical] / 2;

	mHandleRect.shrinkFromCenter(mHandlePadding, true);
}

const RectF& ScrollableBarWidget::getHandleRect() const {
	return mHandleRect;
}