
#include "SimpleWidgets.hpp"

using namespace tp;

void LabelWidget::setText(const std::string& text) {
	mText = text;
}

const std::string& LabelWidget::getText() const {
	return mText;
}

void LabelWidget::draw(Canvas& canvas) {
	canvas.text(mText.c_str(), getRelativeArea(), mSize, Canvas::LC, mPadding, mColor);
}

ButtonWidget::ButtonWidget() {
	mAction = []() {
		printf("Button Pressed!\n");
	};

	setDebug("button", { 0.1, 0.1, 0.7, 0.7 });

	mColorAnimated.setTargetColor(mColor);
	mColorAnimated.endAnimation();
}

void ButtonWidget::setAction(const std::function<void()>& action) {
	mAction = action;
}

void ButtonWidget::process(const EventHandler& eventHandler) {
	if (getArea().isInside(eventHandler.getPointer())) {
		if (eventHandler.isPressed(InputID::MOUSE1)) {
			mAction();
		}
	}
}

void ButtonWidget::draw(Canvas& canvas) {
	canvas.rect(getRelativeArea(), mColorAnimated.getCurrentColor(), mRounding);
	LabelWidget::draw(canvas);
}

bool ButtonWidget::needsNextFrame() const {
	return LabelWidget::needsNextFrame() || !mColorAnimated.shouldEndTransition();
}

void ButtonWidget::endAnimations() {
	mColorAnimated.endAnimation();
	LabelWidget::endAnimations();
}

void ButtonWidget::updateAnimations() {
	mColorAnimated.updateCurrentRect();
	LabelWidget::updateAnimations();
}

void ButtonWidget::mouseEnter() {
	mColorAnimated.setTargetColor(mColor);
	mColorAnimated.endAnimation();

	mColorAnimated.setTargetColor(mColorHovered);
	mColorAnimated.updateCurrentRect();
	triggerWidgetUpdate();
}

void ButtonWidget::mouseLeave() {
	mColorAnimated.setTargetColor(mColor);
	mColorAnimated.updateCurrentRect();
	triggerWidgetUpdate();
}
