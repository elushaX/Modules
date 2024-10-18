#include "FloatingWidget.hpp"
#include "ScrollableLayout.hpp"

using namespace tp;

void FloatingWidget::process(const EventHandler& events) {
	const auto pointer = events.getPointer();

	if (getRelativeAreaT().isInside(pointer) && events.isPressed(InputID::MOUSE1)) {
		layout()->startAction(pointer);

		bringToFront();
		lockFocus();
	}

	if (layout()->isFloating() && events.isReleased(InputID::MOUSE1)) {
		layout()->endAction();
		freeFocus();
	}

	layout()->updateAction(pointer);
}

void FloatingWidget::draw(Canvas& canvas) {
	canvas.rect(layout()->resizeHandleRect(), RGBA(0.7f), 2);
	canvas.rect(getRelativeArea(), RGBA(0.5f), 10);
}

bool FloatingWidget::processesEvents() const {
	return true;
}

bool FloatingWidget::propagateEventsToChildren() const {
	return !layout()->isFloating();
}

bool FloatingWidget::isFloating() const {
	return layout()->isFloating();
}

bool FloatingWidget::needsNextFrame() const {
	return Widget::needsNextFrame() || isFloating();
}

FloatingLayout* FloatingWidget::layout() { return dynamic_cast<FloatingLayout*>(Widget::getLayout()); }

const FloatingLayout* FloatingWidget::layout() const {
	return dynamic_cast<const FloatingLayout*>(Widget::getLayout());
}

FloatingMenu::FloatingMenu() : FloatingWidget() {
	setDebug("float menu", { 0.0, 0.9, 0.1, 0.7 });

	// addChild(&mMenuLayout);

	addChild(&mHeader);
	addChild(&mBodyLayout);

	mHeader.setText("Menu");

	mHeader.setSizePolicy(SizePolicy::Expanding, SizePolicy::Minimal);
	mBodyLayout.setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);

	mBodyLayout.setLayout(new ScrollableLayout(&mBodyLayout));
	mBodyLayout.addChild(&mScrollBar);
	mBodyLayout.addChild(&mContentWidget);

	mContentWidget.setSizePolicy(SizePolicy::Minimal, SizePolicy::Minimal);

	// getLayout()->setLayoutPolicy(LayoutPolicy::Vertical);
	// mBodyLayout.getLayout()->setLayoutPolicy(LayoutPolicy::Vertical);
}

void FloatingMenu::setText(const std::string& text) {
	mHeader.setText(text);
}
