#include "SplitViewWidget.hpp"

using namespace tp;

SplitView::SplitView() {
	mLeftLabel.mLabel = "Left";
	mRightLabel.mLabel = "Right";

	this->mChildWidgets.pushBack(&mLeftLabel);
	this->mChildWidgets.pushBack(&mRightLabel);
}

void SplitView::eventProcess(const Events& events) {
	mIsHover = getHandle().isInside(events.getPointer());

	if (events.isPressed(InputID::MOUSE1) && mIsHover) {
		mResizeInProcess = true;
	} else if (events.isReleased(InputID::MOUSE1)) {
		mResizeInProcess = false;
	}

	if (mResizeInProcess) {
		halnf pos = events.getPointer().x;
		auto diff = pos - (this->mArea.x + mFactor * this->mArea.z);
		mFactor += diff / this->mArea.z;
	}

	mFactor = tp::clamp(mFactor, mMinSize / this->mArea.z, 1 - mMinSize / this->mArea.z);

	if (mMinSize * 2.f > this->mArea.z) {
		mFactor = 0.5f;
	}

	mLeftLabel.mEnable = mHeaders && mCollapsedSide != LEFT;
	mRightLabel.mEnable = mHeaders && mCollapsedSide != RIGHT;

	if (mHeaders) {
		mLeftLabel.setArea(getFirstHeader());
		mRightLabel.setArea(getSecondHeader());
	}

	if (mLeftLabel.isReleased()) {
		mCollapsedSide = (mCollapsedSide == NONE) ? RIGHT : NONE;
	}

	if (mRightLabel.isReleased()) {
		mCollapsedSide = (mCollapsedSide == NONE) ? LEFT : NONE;
	}
}

void SplitView::eventDraw(Canvas& canvas) {
	if (mResizeInProcess) canvas.rect(getHandle(), mResizingColor);
	else if (mIsHover) canvas.rect(getHandle(), mHoveredColor);
	else canvas.rect(getHandle(), mHandleColor);
}

bool SplitView::getFirstEnabled() const { return mCollapsedSide != CollapsedSize::LEFT; }

bool SplitView::getSecondEnabled() const { return mCollapsedSide != CollapsedSize::RIGHT; }

RectF SplitView::getFirst() const {
	Rect out = { this->mArea.x, this->mArea.y, mFactor * this->mArea.z - mHandleSize / 2.f, this->mArea.w };

	if (mCollapsedSide == RIGHT) {
		out.pos.x = this->mArea.pos.x;
		out.size.x = this->mArea.size.x;
	}

	if (mHeaders) {
		out.pos.y += mHeaderSize;
		out.size.y -= mHeaderSize;
	}

	return out;
}

RectF SplitView::getFirstHeader() const {
	Rect out = { this->mArea.x, this->mArea.y, mFactor * this->mArea.z - mHandleSize / 2.f, mHeaderSize };

	if (mCollapsedSide == RIGHT) {
		out.pos.x = this->mArea.pos.x;
		out.size.x = this->mArea.size.x;
	}

	return out;
}

RectF SplitView::getSecondHeader() const {
	RectF out = { this->mArea.x + mFactor * this->mArea.z + mHandleSize / 2.f,
								this->mArea.y,
								(1.f - mFactor) * this->mArea.z - mHandleSize / 2.f,
								mHeaderSize };

	if (mCollapsedSide == LEFT) {
		out.pos.x = this->mArea.pos.x;
		out.size.x = this->mArea.size.x;
	}

	return out;
}

RectF SplitView::getSecond() const {
	RectF out = { this->mArea.x + mFactor * this->mArea.z + mHandleSize / 2.f,
								this->mArea.y,
								(1.f - mFactor) * this->mArea.z - mHandleSize / 2.f,
								this->mArea.w };

	if (mCollapsedSide == LEFT) {
		out.pos.x = this->mArea.pos.x;
		out.size.x = this->mArea.size.x;
	}

	if (mHeaders) {
		out.pos.y += mHeaderSize;
		out.size.y -= mHeaderSize;
	}

	return out;
}

RectF SplitView::getHandle() const {
	if (mCollapsedSide != NONE) return {};
	return { this->mArea.x + mFactor * this->mArea.z - mHandleSize / 2.f, this->mArea.y, mHandleSize, this->mArea.w };
}

void SplitView::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("SplitView");

	mHandleColor = wm.getColor("Handle", "Accent");
	mHoveredColor = wm.getColor("Hovered", "Interaction");
	mResizingColor = wm.getColor("Resizing", "Action");
	mMinSize = wm.getNumber("Min", 200.f);
	mHandleSize = wm.getNumber("HandleSize", 7.f);
	mHeaderSize = wm.getNumber("HeaderSize", 30.f);
}
