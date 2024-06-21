#pragma once

#include "LabelWidget.hpp"

namespace tp {

	//TODO : clean up this mess
	// takes the whole parent area
	template <typename Events, typename Canvas>
	class SplitView : public Widget<Events, Canvas> {
	public:
		SplitView() {
			mLeftLabel.mLabel = "Left";
			mRightLabel.mLabel = "Right";

			this->mChildWidgets.pushBack(&mLeftLabel);
			this->mChildWidgets.pushBack(&mRightLabel);
		}

		void eventProcess(const Events& events) override {
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

		void eventDraw(Canvas& canvas) override {
			if (mResizeInProcess) canvas.rect(getHandle(), mResizingColor);
			else if (mIsHover) canvas.rect(getHandle(), mHoveredColor);
			else canvas.rect(getHandle(), mHandleColor);
		}

		[[nodiscard]] bool getFirstEnabled() const {
			return mCollapsedSide != CollapsedSize::LEFT;
		}

		[[nodiscard]] bool getSecondEnabled() const {
			return mCollapsedSide != CollapsedSize::RIGHT;
		}

		RectF getFirst() const {
			Rect out =  { this->mArea.x, this->mArea.y, mFactor * this->mArea.z - mHandleSize / 2.f, this->mArea.w };

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

		RectF getFirstHeader() const {
			Rect out =  { this->mArea.x, this->mArea.y, mFactor * this->mArea.z - mHandleSize / 2.f, mHeaderSize };

			if (mCollapsedSide == RIGHT) {
				out.pos.x = this->mArea.pos.x;
				out.size.x = this->mArea.size.x;
			}

			return out;
		}

		RectF getSecondHeader() const {
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

		RectF getSecond() const {
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

		RectF getHandle() const {
			if (mCollapsedSide != NONE) return {};
			return { this->mArea.x + mFactor * this->mArea.z - mHandleSize / 2.f, this->mArea.y, mHandleSize, this->mArea.w };
		}

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("SplitView");

			mHandleColor = wm.getColor("Handle", "Accent");
			mHoveredColor = wm.getColor("Hovered", "Interaction");
			mResizingColor = wm.getColor("Resizing", "Action");
			mMinSize = wm.getNumber("Min", 200.f);
			mHandleSize = wm.getNumber("HandleSize", 7.f);
			mHeaderSize = wm.getNumber("HeaderSize", 30.f);
		}

	public:
		halnf mFactor = 0.7f;
		bool mResizeInProcess = false;
		bool mIsHover = false;

		RGBA mHandleColor;
		RGBA mHoveredColor;
		RGBA mResizingColor;
		halnf mMinSize = 0;
		halnf mHandleSize = 0;

		// Headers
		halnf mHeaderSize = 0;
		bool mHeaders = true;
		enum CollapsedSize { NONE, LEFT, RIGHT } mCollapsedSide = NONE;

		LabelWidget<Events, Canvas> mLeftLabel;
		LabelWidget<Events, Canvas> mRightLabel;
	};
}