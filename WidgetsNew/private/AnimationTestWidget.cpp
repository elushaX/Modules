#include "AnimationTestWidget.hpp"

using namespace tp;

void AnimationTestWidget::process(const EventHandler& events) {
	if (events.isDown(tp::InputID::MOUSE1)) {
		mTestSpring.getStart().setTargetPosition(events.getPointer());
	}

	if (events.isDown(tp::InputID::MOUSE2)) {
		mTestSpring.getEnd().setTargetPosition(events.getPointer());
	}

	mTestSpring.updateCurrentRect();

	if (mTestSpring.shouldEndTransition()) {
		mTestSpring.endAnimation();
	}
}

void AnimationTestWidget::draw(Canvas& canvas) {
	canvas.rect(getRelativeArea(), { 0, 0, 0, 1 }, 10);
	canvas.rect(mTestSpring.getCurrentRect(), RGBA(1.f), 10);
}

bool AnimationTestWidget::needsNextFrame() const {
	return Widget::needsNextFrame() || !mTestSpring.shouldEndTransition();
}
