#include "LabelWidget.hpp"

using namespace tp;

LabelWidget::LabelWidget() { this->mArea = { 0, 0, 100, 30 }; }

void LabelWidget::eventDraw(Canvas& canvas) {
	canvas.text(mLabel.c_str(), this->mArea, fontSize, Canvas::LC, padding, fontColor);
}

void LabelWidget::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("Label");

	fontSize = wm.getNumber("Size", "FontSize");
	padding = wm.getNumber("Padding", "Padding");
	fontColor = wm.getColor("Default", "Front");
}
