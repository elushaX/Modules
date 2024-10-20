#include "ColorPickerWidget.hpp"

using namespace tp;

void RGBPickerWidget::process(const EventHandler& events) {
	if (events.isPressed(InputID::MOUSE1)) {
		mColorWheel.fromPoint(getArea().relative(), events.getPointer());
		lockFocus();
	}

	if (events.isReleased(InputID::MOUSE1)) {
		freeFocus();
	}

	if (events.getPointerDelta().length() > EPSILON && events.isDown(InputID::MOUSE1)) {
		mColorWheel.fromPoint(getArea().relative(), events.getPointer());
	}
}

void RGBPickerWidget::draw(Canvas& canvas) {
	canvas.colorWheel(getArea().relative(), mColorWheel);
}
