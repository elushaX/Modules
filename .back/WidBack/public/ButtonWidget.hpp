#pragma once

#include "LabelWidget.hpp"

#include <functional>

namespace tp {

	class ButtonWidget : public Widget {
	public:
		// enum State { NONE, ANTICIPATION, ACTIVATED, CONFIRMED };

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
		// State mStat = NONE;

		RGBA pressedColor;
		RGBA hoveredColor;
		RGBA accentColor;
		halnf rounding = 0;

		std::function<void()> mCallback = [](){};
	};
}