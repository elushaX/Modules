#include "ScrollableWidget.hpp"

using namespace tp;

ScrollBarWidget::ScrollBarWidget() = default;

void ScrollBarWidget::eventProcess(const Events& events) {
	auto area = getHandle();
	mHovered = getHandleHandle().isInside(events.getPointer());

	if (mSizeFraction > 1.f) {
		mPositionFraction = 0;
		return;
	}

	if (events.getScrollY() != 0 && mArea.isInside(events.getPointer())) {
		auto offset = events.getScrollY() < 0 ? 1.0f : -1.0f;
		mPositionFraction += mSizeFraction * offset * 0.3f;
		mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
	}

	if (events.isPressed(InputID::MOUSE1) && area.isInside(events.getPointer())) {
		mIsScrolling = true;
	} else if (events.isReleased(InputID::MOUSE1)) {
		mIsScrolling = false;
	}

	if (mIsScrolling) {
		tp::halnf pos = events.getPointer().y;
		pos = (pos - area.y - mSizeFraction * area.w / 2.f) / area.w;
		mPositionFraction = tp::clamp(pos, 0.f, 1.f - mSizeFraction);
	}

	mPositionFraction = tp::clamp(mPositionFraction, 0.f, 1.f - mSizeFraction);
}

void ScrollBarWidget::eventDraw(Canvas& canvas) {
	auto area = getHandle();

	if (mSizeFraction > 1.f) return;
	// if (!areaParent.isOverlap(getHandle())) return;

	tp::RGBA col = mHandleColor;

	if (mIsScrolling) {
		col = mScrollingColor;
	} else if (mHovered) {
		col = mHoveredColor;
	}

	canvas.rect(area, mDefaultColor, mRounding);
	canvas.rect(getHandleHandle(), col, mRounding);
}

RectF ScrollBarWidget::getHandleHandle() const {
	auto area = getHandle();
	auto sliderSize = tp::clamp(area.w * mSizeFraction, mMinSize * 2, area.w);
	auto diffSize = sliderSize - area.w * mSizeFraction;
	return { area.x, area.y + (area.w - diffSize) * mPositionFraction, area.z, sliderSize };
}

RectF ScrollBarWidget::getViewport() const {
	if (mSizeFraction > 1.f) {
		return this->mArea;
	}
	return { this->mArea.x, this->mArea.y, this->mArea.z - mHandleSize, this->mArea.w };
}

RectF ScrollBarWidget::getHandle() const {
	return { this->mArea.x + this->mArea.z - mHandleSize + mPadding,
					 this->mArea.y + mPadding,
					 mHandleSize - mPadding * 2,
					 this->mArea.w - mPadding * 2 };
}

void ScrollBarWidget::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("Scrollbar");

	mDefaultColor = wm.getColor("Default", "Base");
	mHandleColor = wm.getColor("Handle", "Accent");
	mHoveredColor = wm.getColor("Hovered", "Interaction");
	mScrollingColor = wm.getColor("Scrolling", "Action");
	mPadding = wm.getNumber("Padding", "Padding");
	mHandleSize = wm.getNumber("HandleSize", 20.f);
	mMinSize = wm.getNumber("MinSize", 20.f);
	mRounding = wm.getNumber("Rounding", "Rounding");
}

ScrollableWindow::ScrollableWindow() {
	this->mChildWidgets.pushBack(&mScroller);
	this->mChildWidgets.pushBack(&mContentWidget);
}

ScrollableWindow::~ScrollableWindow() = default;

void ScrollableWindow::eventProcess(const Events& events) {
	List<Widget*>& content = mContentWidget.mChildWidgets;

	// to account all changed geometry of child widgets
	for (auto widget : content) {
		widget->procWrapper(events, mContentWidget.mVisibleArea);
	}

	updateContents(content);
	updateContentSize(content);

	const auto padding = mPadding;

	mScroller.mSizeFraction = mContentWidget.mArea.w / mContentSize;
	mScroller.setArea(this->mArea);
	mContentWidget.setArea(mScroller.getViewport());

	if (mScroller.mSizeFraction > 1.f) {
		setOffset(content, 0);
	} else {
		setOffset(content, (-mScroller.mPositionFraction) * mContentSize);
	}

	for (auto widget : content) {
		widget->setArea({ mContentWidget.mArea.x + padding,
											mContentWidget.mArea.y + widget->mArea.y,
											mScroller.getViewport().z - padding * 2,
											widget->mArea.w });
	}
}

void ScrollableWindow::addWidget(Widget* widget) {
	mContentWidget.mChildWidgets.pushBack(widget);

	List<Widget*>& content = mContentWidget.mChildWidgets;
	updateContents(content);
	updateContentSize(content);
}

void ScrollableWindow::clearContent() { mContentWidget.mChildWidgets.removeAll(); }
List<Widget*>& ScrollableWindow::getContent() { return mContentWidget.mChildWidgets; }

void ScrollableWindow::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("ScrollableWidget");
	mPadding = wm.getNumber("Padding", "Padding");
}

[[nodiscard]] halnf ScrollableWindow::getContentSize() const { return mContentSize; }

void ScrollableWindow::updateContents(List<Widget*>& contentWidgets) {
	if (!contentWidgets.size()) {
		return;
	}

	const halnf offset = contentWidgets.first()->mArea.y + mPadding;

	halnf start = 0;
	for (auto widget : contentWidgets) {
		widget->mArea.y = start;
		start += widget->mArea.w + mPadding;
	}

	for (auto widget : contentWidgets) {
		widget->mArea.y += offset;
	}
}

void ScrollableWindow::updateContentSize(List<Widget*>& contentWidgets) {
	mContentSize = 0;
	if (contentWidgets.size()) {
		mContentSize = contentWidgets.last()->mArea.y - contentWidgets.first()->mArea.y;
		mContentSize += contentWidgets.last()->mArea.w;
		mContentSize += 2 * mPadding;
	}
}

void ScrollableWindow::setOffset(List<Widget*>& contentWidgets, const halnf offset) {
	if (!contentWidgets.size()) return;
	auto newOffset = offset - contentWidgets.first()->mArea.y + mPadding;
	for (auto widget : contentWidgets) {
		widget->mArea.y += newOffset;
	}
}
