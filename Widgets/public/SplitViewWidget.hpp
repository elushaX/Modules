#pragma once

#include "WidgetBase.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class SplitView : public Widget<Events, Canvas> {
	public:
		SplitView() { this->mId = "SplitView"; }

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);

			if (!this->mVisible) {
				mResizeInProcess = false;
				return;
			}

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

		// takes whole area
		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

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

		void setupConfig(WidgetManager& wm) {
			if (!wm.createWidgetConfig(this->mId)) return;
			wm.addReference(this->mId, "Handle", "Accent");
			wm.addReference(this->mId, "Hovered", "Interaction");
			wm.addReference(this->mId, "Resizing", "Action");
			wm.addNumber(this->mId, "Min", 200.f);
			wm.addNumber(this->mId, "HandleSize", 7.f);
		}

		void updateConfigCache(const WidgetManager& wm) override {
			mHandleColor = wm.getColor(this->mId, "Handle");
			mHoveredColor = wm.getColor(this->mId, "Hovered");
			mResizingColor = wm.getColor(this->mId, "Resizing");
			mMinSize = wm.getNumber(this->mId, "Min");
			mHandleSize = wm.getNumber(this->mId, "HandleSize");
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