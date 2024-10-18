#pragma once

#include "WidgetBase.hpp"

namespace tp {

	class LabelWidget : public Widget {
	public:
		LabelWidget();

		void eventDraw(Canvas& canvas) override;

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	public:
		std::string mLabel = "Label";

		halnf fontSize = 10;
		halnf padding = 0;
		RGBA fontColor = { 1, 1, 1, 1 };
	};
}