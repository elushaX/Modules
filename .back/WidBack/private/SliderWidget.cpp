#include "SliderWidget.hpp"

using namespace tp;

SliderWidget::SliderWidget() = default;

void SliderWidget::eventProcess(const Events& events) {
	if (this->isPressed()) {
		mIsSliding = true;
	} else if (events.isReleased(InputID::MOUSE1)) {
		mIsSliding = false;
	}

	if (mIsSliding) {
		mFactor = (events.getPointer().x - this->mArea.x - handleSize / 2.f) / (this->mArea.z - handleSize);
	}

	mFactor = tp::clamp(mFactor, 0.f, 1.f);
}

void SliderWidget::eventDraw(Canvas& canvas) {
	canvas.rect(this->mArea, handleColor, rounding);
	canvas.rect(this->mArea.shrink(borderSize), defaultColor, rounding);
	canvas.rect(getHandle(), handleColor, rounding);
}

RectF SliderWidget::getHandle() const {
	const auto left = this->mArea.x + (this->mArea.z - handleSize) * mFactor;
	return { left, this->mArea.y, handleSize, this->mArea.w };
}

void SliderWidget::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("Slider");
	defaultColor = wm.getColor("Default", "Base");
	handleColor = wm.getColor("Handle", RGBA(0.3f, 0.3f, 0.3f, 1.f));
	borderSize = wm.getNumber("BorderSize", 2);
	handleSize = wm.getNumber("HandleSize", 15.f);
	rounding = wm.getNumber("Rounding", "Rounding");
}

NamedSliderWidget::NamedSliderWidget(const char* name) {
	mLabel.mLabel = name;
	this->mArea = { 0, 0, 100, 30 };

	this->mChildWidgets.pushBack(&mSlider);
	this->mChildWidgets.pushBack(&mLabel);
}

void NamedSliderWidget::eventProcess(const Events& events) {
	const auto widthFirst = this->mArea.z * mFactor;
	const auto widthSecond = this->mArea.z * (1.f - mFactor);

	RectF rec = this->mArea;
	rec.size.x = widthFirst;

	mLabel.setArea(rec);

	rec.pos.x += widthFirst;
	rec.size.x = widthSecond;

	mSlider.setArea(rec);
}
