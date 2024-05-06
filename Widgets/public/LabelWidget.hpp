#pragma once

#include "WidgetBase.hpp"

namespace tp {
	template <typename Events, typename Canvas>
	class LabelWidget : public Widget<Events, Canvas> {
	public:
		LabelWidget() = default;

		void drawCallback(Canvas& canvas) override {
			canvas.text(mLabel.c_str(), this->mArea, fontSize, Canvas::CC, padding, fontColor);
		}

	public:
		void updateConfigCallback(WidgetManager& wm) override {
			wm.setActiveId("Label");

			fontSize = wm.getNumber("Size", "FontSize");
			padding = wm.getNumber("Padding", "Padding");
			fontColor = wm.getColor("Default", "Front");
		}

	public:
		std::string mLabel = "Label";

		halnf fontSize = 10;
		halnf padding = 0;
		RGBA fontColor = { 1, 1, 1, 1 };
	};
}