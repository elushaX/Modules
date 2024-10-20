
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

void ButtonWidget::setColor(const RGBA& in) {
	mColor = in;
	mColorAnimated.setTargetColor(mColor);
	triggerWidgetUpdate("color changed");
}

void ButtonWidget::process(const EventHandler& eventHandler) {
	if (getRelativeArea().isInside(eventHandler.getPointer())) {
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

	// mColorAnimated.updateCurrentRect();
	triggerWidgetUpdate("button hovered");
}

void ButtonWidget::mouseLeave() {
	mColorAnimated.setTargetColor(mColor);
	//mColorAnimated.updateCurrentRect();

	triggerWidgetUpdate("button out of focus");
}


void SliderWidget::process(const EventHandler& events) {
	const auto pointer = events.getPointer();

	switch (mState) {
		case SLIDING:
			if (events.isReleased(InputID::MOUSE1)) {
				mState = IDLE;
				freeFocus();
			}

			mFactor = ((pointer - mHandleSize / 2) / (getArea().relative().size - mHandleSize)).x;
			mFactor = clamp(mFactor, 0.f, 1.f);
			break;

		case IDLE:
		case HOVER:
			mState = getHandleArea().isInside(pointer) ? HOVER : IDLE;
			if (getRelativeAreaT().isInside(pointer) && events.isPressed(InputID::MOUSE1)) {
				mState = SLIDING;
				lockFocus();
			}
			break;
	}
}

void SliderWidget::draw(Canvas& canvas) {
	canvas.rect(getArea().relative(), mColorBG, mRounding);

	switch (mState) {
		case IDLE: canvas.rect(getHandleArea(), mColorIdle, mRounding); break;
		case HOVER: canvas.rect(getHandleArea(), mColorHovered, mRounding); break;
		case SLIDING: canvas.rect(getHandleArea(), mColorActive, mRounding); break;
	}
}

RectF SliderWidget::getHandleArea() const {
	auto area = getArea().relative();

	const auto& size = area.size;
	auto center = area.center();
	auto tilt = -(mFactor - 0.5f) * 2;

	area.pos.x = center.x - (tilt * ((size.x - mHandleSize) / 2)) - mHandleSize / 2;
	area.size.x = mHandleSize;

	return area;
}

void PopupWidget::open(Widget* parent, const RectF& at) {
	mParentArea = parent->getArea().relative();
	mParentArea.pos = at.pos * -1;
	mParentArea.shrinkFromCenter(-10, true);

	setArea(at);
	parent->openPopup(this);
}

void PopupWidget::process(const EventHandler& events) {
	bool insideParent = mParentArea.isInside(events.getPointer());
	bool insideMenu = getArea().relative().isInside(events.getPointer());

	if (!(insideMenu || insideParent)) {
		closePopup(this);
	}
}

void PopupWidget::draw(Canvas& canvas) {
	canvas.rect(getArea().relative(), col, rounding);
}

void HoverPopupTriggerWidget::mouseEnter() {
	auto area = getArea().relative();
	auto size = Vec2F(300, 400);

	if (mDirection == Right) {
		auto popupArea = RectF{ area.p4() + Vec2F{ gap, 0.f }, size };
		mPopup.open(this, popupArea);
	} else {
		auto popupArea = RectF{ area.p2() + Vec2F{ 0.f, gap }, size };
		mPopup.open(this, popupArea);
	}
}

PopupWidget* HoverPopupTriggerWidget::getPopup() {
	return &mPopup;
}

void HoverPopupTriggerWidget::draw(Canvas& canvas) {
	canvas.rect(getArea().relative(), col, rounding);
	LabelWidget::draw(canvas);
}
