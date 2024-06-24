#pragma once

#include "LabelWidget.hpp"

namespace tp {

	class ButtonWidget : public Widget {
	public:
		ButtonWidget();
		ButtonWidget(const std::string& label, const tp::RectF& aArea);
		bool isFired();
		void eventProcess(const Events&) override;
		void eventDraw(Canvas& canvas) override;
		void setLabel(const std::string& string);

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	public:
		LabelWidget mLabel;

		RGBA pressedColor;
		RGBA hoveredColor;
		RGBA accentColor;
		halnf rounding = 0;
	};
}