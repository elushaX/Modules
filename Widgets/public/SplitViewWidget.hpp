#pragma once

#include "WidgetBase.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class SplitView : public Widget<Events, Canvas> {
	public:
		SplitView() = default;

		// takes whole area
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
		}

		void eventDraw(Canvas& canvas) override {
			if (mResizeInProcess) canvas.rect(getHandle(), mResizingColor);
			else if (mIsHover) canvas.rect(getHandle(), mHoveredColor);
			else canvas.rect(getHandle(), mHandleColor);
		}

		RectF getFirst() const {
			return { this->mArea.x, this->mArea.y, mFactor * this->mArea.z - mHandleSize / 2.f, this->mArea.w };
		}

		RectF getSecond() const {
			return { this->mArea.x + mFactor * this->mArea.z + mHandleSize / 2.f,
							 this->mArea.y,
							 (1.f - mFactor) * this->mArea.z - mHandleSize / 2.f,
							 this->mArea.w };
		}

		RectF getHandle() const {
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
	};
}