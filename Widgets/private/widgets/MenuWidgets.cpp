#include "MenuWidgets.hpp"

#include "ScrollableLayout.hpp"

using namespace tp;

CollapsingMenuWidget::CollapsingMenuWidget() {
	setDebug("Collapsing Menu", {1});

	setSizePolicy(SizePolicy::Expanding, SizePolicy::Minimal);

	addChild(&mHeader);
	addChild(&mContent);
}

void CollapsingMenuWidget::process(const EventHandler& events) {
	mHover = mHeader.getArea().isInside(events.getPointer());

	if (events.isPressed(InputID::MOUSE1) && mHover) setCollapsed(!mContent.getEnabled());
}

void CollapsingMenuWidget::setCollapsed(bool collapsed) {
	mContent.setEnabled(collapsed);
}

void CollapsingMenuWidget::draw(Canvas& canvas) {
	canvas.rect(getArea().relative(), colorBG, rounding);
	if (mHover) {
		canvas.rect(mHeader.getArea(), colorHover, rounding);
	}
}

FloatingScrollableMenu::FloatingScrollableMenu() : FloatingWidget() {
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

void FloatingScrollableMenu::setText(const std::string& text) {
	mHeader.setText(text);
}
