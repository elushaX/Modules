#include "FloatingWidget.hpp"

using namespace tp;

void FloatingWidget::process(const EventHandler& events) {
	const auto relativePointer = events.getPointer();

	bool inside = getRelativeArea().isInside(relativePointer);

	if (inside && events.isPressed(InputID::MOUSE1)) {
		mPointerStart = relativePointer;
		mIsFloating = true;
	}

	if (mIsFloating && events.isReleased(InputID::MOUSE1)) {
		mIsFloating = false;
	}

	mPointerCurrent = relativePointer;

	setActive(mIsFloating || needUpdate());
}

void FloatingWidget::updateArea(RectF& area) const {
	if (mIsFloating) {
		area.pos += mPointerCurrent - mPointerStart;
	}
}

void FloatingWidget::draw(Canvas& canvas) {
	canvas.rect(getRelativeArea(), RGBA(0.5f), 10);
}