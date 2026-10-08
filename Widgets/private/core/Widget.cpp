#include "Widget.hpp"

#include "Animation.hpp"
#include "Event.hpp"
#include "Layout.hpp"
#include "Painter.hpp"
#include "Ui.hpp"
#include "WidgetInternals.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

Widget::Widget() = default;

Widget::~Widget() {
	if (mParent) mParent->removeChild(*this);
	else if (mUi && mUi->root() == this) mUi->setRoot(nullptr);
	else detachSubtree();

	for (Widget* child : mChildren) {
		child->detachSubtree();
		child->mParent = nullptr;
	}
}

void Widget::addChild(Widget& child) { insertChild(mChildren.size(), child); }

void Widget::insertChild(size_t index, Widget& child) {
	if (&child == this || child.isSelfOrAncestorOf(*this)) return;

	if (child.mParent) child.mParent->removeChild(child);
	if (child.mUi && child.mUi->root() == &child) child.mUi->setRoot(nullptr);

	index = std::min(index, mChildren.size());
	mChildren.insert(mChildren.begin() + (long) index, &child);
	child.mParent = this;

	if (mUi) child.attachSubtree(*mUi);
	requestLayout();
}

void Widget::removeChild(Widget& child) {
	const auto position = std::find(mChildren.begin(), mChildren.end(), &child);
	if (position == mChildren.end()) return;

	child.detachSubtree();
	mChildren.erase(position);
	child.mParent = nullptr;

	requestLayout();
}

void Widget::removeAllChildren() {
	while (!mChildren.empty())
		removeChild(*mChildren.back());
}

void Widget::bringToFront() {
	if (!mParent) return;

	auto& siblings = mParent->mChildren;
	siblings.erase(std::find(siblings.begin(), siblings.end(), this));
	siblings.push_back(this);
	requestPaint();
}

bool Widget::isSelfOrAncestorOf(const Widget& other) const {
	for (const Widget* iterator = &other; iterator; iterator = iterator->mParent) {
		if (iterator == this) return true;
	}
	return false;
}

void Widget::setSizing(const Sizing& sizing) {
	if (sizing.horizontal == mSizing.horizontal && sizing.vertical == mSizing.vertical) return;

	mSizing = sizing;
	requestLayout();
}

void Widget::setSizing(const AxisSizing& horizontal, const AxisSizing& vertical) {
	setSizing(Sizing{ horizontal, vertical });
}

void Widget::setSizing(Axis axis, const AxisSizing& sizing) {
	Sizing updated = mSizing;
	updated.along(axis) = sizing;
	setSizing(updated);
}

void Widget::setLayout(std::unique_ptr<Layout> layout) {
	mLayout = std::move(layout);
	if (mLayout) mLayout->mOwner = this;

	requestLayout();
}

void Widget::requestPosition(Vec2F position) {
	if (mRequestedPosition == position) return;

	mRequestedPosition = position;
	if (mParent) mParent->requestLayout();
}

Vec2F Widget::windowPosition() const {
	Vec2F position = mRect.pos;
	for (const Widget* ancestor = mParent; ancestor; ancestor = ancestor->mParent)
		position += ancestor->mRect.pos;
	return position;
}

void Widget::setGeometryAnimated(bool animated) {
	if (animated == mGeometryAnimated) return;

	mGeometryAnimated = animated;
	if (!animated) finishGeometryAnimation();
}

bool Widget::isGeometryMoving() const { return mGeometry && mGeometry->isAnimating(); }

void Widget::animateGeometryFrom(const RectF& start) {
	if (!mGeometryAnimated || !mPlaced) return;

	if (!mGeometry) {
		mGeometry = std::make_unique<AnimatedRect>(start, Motion::fluid(), AnimatedRect::kGeometryTolerance);
	}
	mGeometry->jumpTo(start);
	mGeometry->animateTo(mTargetRect);
	mRect = start;

	if (mUi) mUi->startGeometryAnimation(*this);
	WidgetInternals::markArrangeNeeded(*this);
}

void Widget::appearFrom(const RectF& start) {
	if (mPlaced) {
		animateGeometryFrom(start);
		return;
	}

	mAppearFrom = start;
	mAppearing = true;
	requestLayout();
}

void Widget::finishGeometryAnimation() {
	finishTransitions();
	if (mGeometry) mGeometry->jumpTo(mTargetRect);
	if (mRect == mTargetRect) return;

	mRect = mTargetRect;
	WidgetInternals::markArrangeNeeded(*this);
}

void Widget::setVisible(bool visible) {
	if (mVisible == visible) return;

	mVisible = visible;
	if (!visible && mUi) mUi->onWidgetHidden(*this);
	if (!visible) WidgetInternals::resetPlacement(*this);
	if (visible && mUi && mParent && mParent->mPlaced) mAppearing = true;
	if (mParent) mParent->requestLayout();
	requestPaint();
}

void Widget::setCursor(CursorShape cursor) {
	if (mCursor == cursor) return;

	mCursor = cursor;
	if (mUi) mUi->refreshCursor();
}

void Widget::setEnabled(bool enabled) {
	if (mEnabled == enabled) return;

	mEnabled = enabled;
	requestPaint();
}

bool Widget::isEffectivelyEnabled() const {
	for (const Widget* iterator = this; iterator; iterator = iterator->mParent) {
		if (!iterator->mEnabled) return false;
	}
	return true;
}

bool Widget::hasKeyboardFocus() const { return mUi && mUi->keyboardFocus() == this; }

bool Widget::showsFocusRing() const { return hasKeyboardFocus() && mUi->isKeyboardNavigating(); }

void Widget::requestLayout() {
	for (Widget* iterator = this; iterator; iterator = iterator->mParent) {
		iterator->mMeasureCache[(int) Axis::Horizontal].valid = false;
		iterator->mMeasureCache[(int) Axis::Vertical].valid = false;
		iterator->mNeedsLayout = true;
	}
	if (mUi) mUi->scheduleLayout();
}

void Widget::requestPaint() {
	if (mUi) mUi->schedulePaint();
}

const Theme& Widget::theme() const { return mUi ? mUi->theme() : Theme::standard(); }

halnf Widget::textWidth(const std::string& text, halnf fontSize) const {
	return textWidth(text, TextStyle{ fontSize, FontFace::Regular });
}

halnf Widget::textWidth(const std::string& text, const TextStyle& style) const {
	return textMetrics().styledTextWidth(text, style);
}

std::vector<halnf> Widget::prefixWidths(const std::string& text, const TextStyle& style) const {
	return textMetrics().prefixWidths(text, style);
}

const TextMetrics& Widget::textMetrics() const { return mUi ? mUi->textMetrics() : Ui::fallbackTextMetrics(); }

Extent Widget::measureContent(Axis, halnf) const { return {}; }

void Widget::onEvent(Event&) {}

void Widget::onTick(halnf) {}

void Widget::paint(Painter&) {}

void Widget::paintOverlay(Painter&) {}

bool Widget::hitTest(Vec2F) const { return true; }

void Widget::startTicking() {
	mTicking = true;
	if (mUi) mUi->startTicking(*this);
}

void Widget::stopTicking() {
	mTicking = false;
	if (mUi) mUi->stopTicking(*this);
}

void Widget::attachSubtree(Ui& ui) {
	mUi = &ui;
	if (mTicking) ui.startTicking(*this);
	if (isGeometryMoving()) ui.startGeometryAnimation(*this);
	onAttached();

	for (Widget* child : mChildren)
		child->attachSubtree(ui);
}

void Widget::detachSubtree() {
	if (!mUi) return;

	for (Widget* child : mChildren)
		child->detachSubtree();

	onDetached();
	mUi->onWidgetDetached(*this);
	mUi = nullptr;
}
