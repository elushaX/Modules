#pragma once

#include "LabelWidget.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class SliderWidget : public Widget<Events, Canvas> {
	public:
		SliderWidget() { this->mId = "SliderWidget"; }

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
				mFactor = (events.getPointer().x - this->mArea.x - handleSize / 2.f) / (this->mArea.z - handleSize);
			}

			mFactor = tp::clamp(mFactor, 0.f, 1.f);
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			canvas.rect(this->mArea, defaultColor, rounding);
			canvas.rect(getHandle(), handleColor, rounding);
		}

		RectF getHandle() const {
			const auto halfHandle = handleSize / 2.f;
			const auto left = this->mArea.x + (this->mArea.z - handleSize) * mFactor;
			return { left, this->mArea.y, handleSize, this->mArea.w };
		}

	public:
		void setupConfig(WidgetManager& wm) {
			if (!wm.createWidgetConfig(this->mId)) return;
			wm.addReference(this->mId, "Default", "Base");
			wm.addReference(this->mId, "Handle", "Accent");
			wm.addNumber(this->mId, "HandleSize", 20.f);
			wm.addReference(this->mId, "Rounding", "Rounding");
		}

		void updateConfigCache(const WidgetManager& wm) override {
			defaultColor = wm.getColor(this->mId, "Default");
			handleColor = wm.getColor(this->mId, "Handle");
			handleSize = wm.getNumber(this->mId, "HandleSize");
			rounding = wm.getNumber(this->mId, "Rounding");
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
			this->mId = "NamedSliderWidget";
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
	public:
		SliderWidget<Events, Canvas> mSlider;
		LabelWidget<Events, Canvas> mLabel;

		halnf mFactor = 0.5f;
	};
}