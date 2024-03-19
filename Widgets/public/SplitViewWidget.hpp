#pragma once

#include "WidgetBase.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class SplitView : public Widget<Events, Canvas> {
	public:
		SplitView() {
			this->createConfig("SplitView");
			this->addColor("Handle", "Accent");
			this->addColor("Hovered", "Interaction");
			this->addColor("Resizing", "Action");
			this->addValue("Min", 200.f);
			this->addValue("HandleSize", 7.f);
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;

			if (!areaParent.isOverlap(aArea)) {
				mResizeInProcess = false;
				return;
			}

			mIsHover = getHandle().isInside(events.getPointer());

			if (events.isPressed(InputID::MOUSE1) && mIsHover) {
				mResizeInProcess = true;
			} else if (!events.isDown()) {
				mResizeInProcess = false;
			}

			if (mResizeInProcess) {
				halnf pos = events.getPos().x;
				auto diff = pos - (this->mArea.x + mFactor * this->mArea.z);
				mFactor += diff / this->mArea.z;
			}

			mFactor = tp::clamp(mFactor, this->getValue("Min") / this->mArea.z, 1 - this->getValue("Min") / this->mArea.z);

			if (this->getValue("Min") * 2.f > this->mArea.z) {
				mFactor = 0.5f;
			}
		}

		// takes whole area
		void draw(Canvas& canvas) override {
			if (mResizeInProcess) canvas.rect(getHandle(), this->getColor("Resizing"));
			else if (mIsHover) canvas.rect(getHandle(), this->getColor("Hovered"));
			else canvas.rect(getHandle(), this->getColor("Handle"));
		}

		RectF getFirst() const {
			return {
				this->mArea.x, this->mArea.y, mFactor * this->mArea.z - this->getValue("HandleSize") / 2.f, this->mArea.w
			};
		}

		RectF getSecond() const {
			return { this->mArea.x + mFactor * this->mArea.z + this->getValue("HandleSize") / 2.f,
							 this->mArea.y,
							 (1.f - mFactor) * this->mArea.z - this->getValue("HandleSize") / 2.f,
							 this->mArea.w };
		}

		RectF getHandle() const {
			return { this->mArea.x + mFactor * this->mArea.z - this->getValue("HandleSize") / 2.f,
							 this->mArea.y,
							 this->getValue("HandleSize"),
							 this->mArea.w };
		}

	public:
		halnf mFactor = 0.7f;
		bool mResizeInProcess = false;
		bool mIsHover = false;
	};
}