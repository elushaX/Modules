#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class SliderWidget : public Widget<Events, Canvas> {
	public:
		SliderWidget() = default;

		void procBody(const Events& events) override {
			if (events.isPressed(InputID::MOUSE1) && this->mArea.isInside(events.getPointer())) {
				mIsSliding = true;
			} else if (events.isReleased(InputID::MOUSE1)) {
				mIsSliding = false;
			}

			if (mIsSliding) {
				mFactor = (events.getPointer().x - this->mArea.x - handleSize / 2.f) / (this->mArea.z - handleSize);
			}

			mFactor = tp::clamp(mFactor, 0.f, 1.f);
		}

		void drawBody(Canvas& canvas) override {
			canvas.rect(this->mArea, defaultColor, rounding);
			canvas.rect(getHandle(), handleColor, rounding);
		}

		RectF getHandle() const {
			const auto left = this->mArea.x + (this->mArea.z - handleSize) * mFactor;
			return { left, this->mArea.y, handleSize, this->mArea.w };
		}

	public:
		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("Slider");
			defaultColor = wm.getColor("Default", "Base");
			handleColor = wm.getColor("Handle", "Accent");
			handleSize = wm.getNumber("HandleSize", 20.f);
			rounding = wm.getNumber("Rounding", "Rounding");
		}

	public:
		halnf mFactor = 0.f;
		bool mIsSliding = false;

		RGBA defaultColor;
		RGBA handleColor;
		halnf handleSize = 0;
		halnf rounding = 0;
	};

	template <typename Events, typename Canvas>
	class NamedSliderWidget : public Widget<Events, Canvas> {
	public:
		explicit NamedSliderWidget(const char* name = "Value") {
			mLabel.mLabel = name;
			this->mArea = { 0, 0, 100, 30 };
		}

		void procBody(const Events& events) override {
			const auto widthFirst = this->mArea.z * mFactor;
			const auto widthSecond = this->mArea.z * (1.f - mFactor);

			RectF rec = this->mArea;
			rec.size.x = widthFirst;

			mLabel.proc(events, this->mArea, rec);

			rec.pos.x += widthFirst;
			rec.size.x = widthSecond;

			mSlider.proc(events, this->mArea, rec);
		}

		void drawBody(Canvas& canvas) override {
			mSlider.draw(canvas);
			mLabel.draw(canvas);
		}

		virtual void updateConfigCache(WidgetManager& wm) {
			wm.setActiveId("NamedSlider");
			mSlider.updateConfigCache(wm);
			mLabel.updateConfigCache(wm);
		}

	public:
		SliderWidget<Events, Canvas> mSlider;
		LabelWidget<Events, Canvas> mLabel;

		halnf mFactor = 0.5f;
	};
}