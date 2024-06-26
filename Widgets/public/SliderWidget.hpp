#pragma once

#include "LabelWidget.hpp"

namespace tp {

	class SliderWidget : public Widget {
	public:
		SliderWidget();

		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;

		RectF getHandle() const;

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	public:
		halnf mFactor = 0.f;
		bool mIsSliding = false;

		RGBA defaultColor;
		RGBA handleColor;
		halnf handleSize = 0;
		halnf rounding = 0;
		halnf borderSize = 2;
	};

	class NamedSliderWidget : public Widget {
	public:
		explicit NamedSliderWidget(const char* name = "Value");
		void eventProcess(const Events& events) override;

	public:
		SliderWidget mSlider;
		LabelWidget mLabel;

		halnf mFactor = 0.5f;
	};
}