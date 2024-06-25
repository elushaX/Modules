
#include "CollapsableMenu.hpp"

using namespace tp;

CollapsableMenu::CollapsableMenu() {
	this->mChildWidgets.pushBack(&mHeader);
	this->mChildWidgets.pushBack(&mBody);
}

void CollapsableMenu::eventProcess(const Events&) {
	if (mHeader.isReleased()) {
		toggleCollapsed();
	}

	updateGeometry();
}

void CollapsableMenu::eventDraw(Canvas& canvas) {
	canvas.rect(this->mArea, mBorderColor, rounding);
	canvas.rect(this->mArea.shrink(mBorderSize), mMenuColor, rounding);
}

void CollapsableMenu::addWidgetToMenu(Widget* widget) {
	mBody.addWidget(widget);

	EventHandler ev;
	mBody.procWrapper(ev, this->mArea);
	updateGeometry();
}

void CollapsableMenu::setLabel(const std::string& string) { mHeader.mLabel = string; }

void CollapsableMenu::toggleCollapsed() { setCollapsed(!getCollapsed()); }

void CollapsableMenu::setCollapsed(bool collapsed) {
	if (mLocked) return;
	if (collapsed && !mCollapsed) mPrevHeight = this->mArea.size.y;
	if (!collapsed && mCollapsed) this->mArea.size.y = mPrevHeight;
	mCollapsed = collapsed;
}

bool CollapsableMenu::getCollapsed() const { return mCollapsed; }

void CollapsableMenu::updateGeometry() {
	mHeader.setArea(getHeaderRect());

	mBody.mEnable = !mCollapsed;

	if (mCollapsed) {
		this->mArea.size.y = headerHeight;
	} else {
		if (mAdjustHeight) {
			this->mArea.size.y = headerHeight + getBodyRect().size.y + mPadding * 2;
		}

		mBody.setArea(getBodyRect());
	}
}

RectF CollapsableMenu::getHeaderRect() {
	RectF out = { this->mArea.pos, { this->mArea.size.x, headerHeight } };
	return out.shrink(mPadding);
}

RectF CollapsableMenu::getBodyRect() {
	RectF out = { { this->mArea.pos.x, this->mArea.pos.y + headerHeight },
								{ this->mArea.size.x, this->mArea.size.y - headerHeight - mPadding } };

	out.size.y -= mBorderSize * 2;

	if (mAdjustHeight) out.size.y = mBody.getContentSize();

	if (mBody.getContentSize()) {
		out = out.shrink(mPadding);
		out.size.y += mPadding * 2 + 1;
	}

	return out;
}

void CollapsableMenu::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("CollapsableMenu");

	headerHeight = wm.getNumber("HeaderHeight", 35);
	mMenuColor = wm.getColor("MenuColor", RGBA(0, 0, 0, 1.f));
	rounding = wm.getNumber("Rounding", "Rounding");
	mBorderColor = wm.getColor("BorderColor", RGBA(0.16, 0.16, 0.16, 1.f));
	mBorderSize = wm.getNumber("BorderSize", 2.f);
	mPadding = wm.getNumber("Padding", "Padding");
}
