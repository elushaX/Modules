
#include "ButtonWidget.hpp"

using namespace tp;

ButtonWidget::ButtonWidget() {
	this->setArea({ 0, 0, 100, 30 });
	this->mChildWidgets.pushBack(&mLabel);
}

ButtonWidget::ButtonWidget(const std::string& label, const tp::RectF& aArea) {
	this->setArea(aArea);
	mLabel.mLabel = label;
	this->mChildWidgets.pushBack(&mLabel);
}

bool ButtonWidget::isFired() { return this->isReleased(); }

void ButtonWidget::eventProcess(const Events&) { mLabel.setArea(this->mArea); }

void ButtonWidget::eventDraw(Canvas& canvas) {
	if (this->isHolding()) {
		canvas.rect(this->mArea, pressedColor, rounding);
	} else if (this->isFocus()) {
		canvas.rect(this->mArea, hoveredColor, rounding);
	} else {
		canvas.rect(this->mArea, accentColor, rounding);
	}
}

void ButtonWidget::setLabel(const std::string& string) { mLabel.mLabel = string; }

void ButtonWidget::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("Button");

	pressedColor = wm.getColor("Pressed", "Action");
	hoveredColor = wm.getColor("Hovered", "Interaction");
	accentColor = wm.getColor("Default", "Accent");
	rounding = wm.getNumber("Rounding", "Rounding");
}
