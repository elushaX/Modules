
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
}

void ButtonWidget::setAction(const std::function<void()>& action) {
	mAction = action;
}

void ButtonWidget::process(const EventHandler& eventHandler) {
	if (getRelativeArea().isInside(eventHandler.getPointer())) {
		if (eventHandler.isPressed(InputID::MOUSE1)) {
			mAction();
		}

		mColorAnimated.setTargetColor(mColor);
	} else {
		mColorAnimated.setTargetColor(mColorHovered);
	}

	mColorAnimated.updateCurrentRect();

	setActive(needUpdate());
}

void ButtonWidget::draw(Canvas& canvas) {
	canvas.rect(getRelativeArea(), mColorAnimated.getCurrentColor(), mRounding);
	LabelWidget::draw(canvas);
}

bool ButtonWidget::needUpdate() const {
	return !mColorAnimated.checkAnimationShouldEnd();
}

void ButtonWidget::finishAnimations() {
	mColorAnimated.endAnimation();
}