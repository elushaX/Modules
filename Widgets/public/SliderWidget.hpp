#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class SliderWidget : public Widget<Events, Canvas> {
	public:
		SliderWidget() {
			this->createConfig("SliderWidget");
			this->addColor("Default", "Base");
			this->addColor("Handle", "Accent");
			this->addColor("Hovered", "Interaction");
			this->addColor("Scrolling", "Action");
			this->addValue("Padding", "Padding");
			this->addValue("HandleSize", 20.f);
			this->addValue("MinSize", 20.f);
			this->addValue("Rounding", "Rounding");
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			if (events.isPressed(InputID::MOUSE1) && this->mArea.isInside(events.getPointer())) {
				mIsSliding = true;
			} else if (events.isReleased(InputID::MOUSE1)) {
				mIsSliding = false;
			}

			if (mIsSliding) {
				const auto handleSize = this->getValue("HandleSize");
				mFactor = (events.getPointer().x - this->mArea.x - handleSize / 2.f) / (this->mArea.z - handleSize);
			}

			mFactor = tp::clamp(mFactor, 0.f, 1.f);
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			canvas.rect(this->mArea, this->getColor("Default"), this->getValue("Rounding"));
			canvas.rect(getHandle(), this->getColor("Handle"), this->getValue("Rounding"));
		}

		RectF getHandle() const {
			const auto handle = this->getValue("HandleSize");
			const auto halfHandle = handle / 2.f;
			const auto left = this->mArea.x + (this->mArea.z - handle) * mFactor;
			return { left, this->mArea.y, handle, this->mArea.w };
		}

	public:
		halnf mFactor = 0.f;
		bool mIsSliding = false;
	};

	template <typename Events, typename Canvas>
	class NamedSliderWidget : public Widget<Events, Canvas> {
	public:
		NamedSliderWidget(const char* name = "Value") { 
			mLabel.mLabel = name;
			this->mArea = { 0, 0, 100, 30 };
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;

			const auto widthFirst = this->mArea.z * mFactor;
			const auto widthSecond = this->mArea.z * (1.f - mFactor);

			RectF rec = this->mArea;
			rec.size.x = widthFirst;

			mLabel.proc(events, this->mArea, rec);

			rec.pos.x += widthFirst;
			rec.size.x = widthSecond;

			mSlider.proc(events, this->mArea, rec);
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			mSlider.draw(canvas);
			mLabel.draw(canvas);
		}

	public:
		SliderWidget<Events, Canvas> mSlider;
		LabelWidget<Events, Canvas> mLabel;

		halnf mFactor = 0.5f;
	};
}