#include "ReorderableList.hpp"

#include "Layout.hpp"
#include "Painter.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kGripDotRadius = 1.3f;
	constexpr halnf kGripDotSpacing = 4;
	constexpr halnf kIndicatorThickness = 2;
	constexpr halnf kLiftedShadowOffset = 3;
}

class ReorderableList::ListLayout : public Layout {
public:
	explicit ListLayout(ReorderableList& list) :
		mList(list) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		if (axis == Axis::Horizontal) {
			Extent widest = { 0, 0, kUnbounded, 1 };
			for (const Widget* child : context.children()) {
				const Extent extent = context.measure(*child, axis, kUnbounded);
				widest.minimum = std::max(widest.minimum, extent.minimum + mList.gripWidth());
				widest.preferred = std::max(widest.preferred, extent.preferred + mList.gripWidth());
			}
			return widest;
		}

		const halnf itemWidth = crossLength >= kUnbounded ? kUnbounded : std::max(0.0f, crossLength - mList.gripWidth());
		halnf total = 0;
		for (const Widget* child : context.children())
			total += context.measure(*child, axis, itemWidth).preferred;
		if (!context.children().empty()) total += mList.mGap * (halnf) (context.children().size() - 1);
		return Extent::rigid(total);
	}

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		const halnf itemWidth = std::max(0.0f, area.size.x - mList.gripWidth());
		const auto& items = mList.mItems;

		std::vector<size_t> order(items.size());
		for (size_t index = 0; index < items.size(); index++)
			order[index] = index;
		if (mList.mDragging) {
			order.erase(order.begin() + (std::ptrdiff_t) mList.mDragged);
			order.insert(order.begin() + (std::ptrdiff_t) mList.mDrop, mList.mDragged);
		}

		halnf y = area.pos.y;
		for (const size_t index : order) {
			Widget& item = *items[index];
			const halnf height = context.measure(item, Axis::Vertical, itemWidth).preferred;
			const halnf top = mList.mDragging && index == mList.mDragged ? mList.mDragY : y;
			context.place(item, makeRect(area.pos.x, top, itemWidth, height));
			y += height + mList.mGap;
		}
	}

private:
	ReorderableList& mList;
};

ReorderableList::ReorderableList(halnf gap) :
	mGap(gap) {
	setDebugName("reorderable list");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	setLayout(std::make_unique<ListLayout>(*this));
}

void ReorderableList::setDragMode(DragMode mode) {
	cancelDrag();
	mDragMode = mode;
	requestLayout();
}

void ReorderableList::addItem(Widget& item) {
	mItems.push_back(&item);
	addChild(item);
}

void ReorderableList::clearItems() {
	cancelDrag();
	if (mPressed || mHeld) {
		if (ui()) ui()->releasePointer(*this);
		mPressed = false;
		mHeld = false;
	}
	mHoveredGrip = -1;
	removeAllChildren();
	mItems.clear();
}

RectF ReorderableList::gripRect(size_t index) const { return gripRectFor(mItems[index]->rect()); }

RectF ReorderableList::gripRectFor(const RectF& row) const {
	const halnf height = std::min(row.size.y, theme().metrics.controlHeight);
	return makeRect(size().x - gripWidth(), row.pos.y, gripWidth(), height);
}

bool ReorderableList::isOnGrip(Vec2F localPoint, size_t index) const { return contains(gripRect(index), localPoint); }

bool ReorderableList::hitTest(Vec2F localPoint) const { return contains(localRect(), localPoint); }

void ReorderableList::onEvent(Event& event) {
	if (event.type() == Event::Type::PointerMove && !mPressed && !mDragging) updateGripHover(event.position());
	handlePointer(event);
}

void ReorderableList::filterDescendantEvent(Event& event) {
	if (!mPressed && !mDragging && hasNestedListAround(event.target())) return;
	if (event.type() == Event::Type::PointerMove && !mPressed && !mDragging)
		updateGripHover(toLocal(event.windowPosition()));
	handlePointer(event);
}

bool ReorderableList::hasNestedListAround(const Widget* target) const {
	for (const Widget* current = target; current && current != this; current = current->parent()) {
		if (dynamic_cast<const ReorderableList*>(current)) return true;
	}
	return false;
}

void ReorderableList::onHoverChanged() {
	if (!isHovered()) updateGripHover({ -1, -1 });
}

void ReorderableList::updateGripHover(Vec2F localPoint) {
	if (!usesGrips()) return;
	long index = itemIndexAt(localPoint);
	if (index >= 0 && !isOnGrip(localPoint, (size_t) index)) index = -1;
	if (index == mHoveredGrip) return;
	mHoveredGrip = index;
	setCursor(index >= 0 ? CursorShape::Grab : CursorShape::Default);
	requestPaint();
}

void ReorderableList::handlePointer(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			{
				if (event.button() != PointerButton::Primary || mDragging) return;
				const Vec2F local = toLocal(event.windowPosition());
				const long index = itemIndexAt(local);
				if (index < 0) return;

				mPressed = true;
				mHeld = false;
				mPressedIndex = (size_t) index;
				mPressLocalY = local.y;
				mPressedOnGrip = usesGrips() && isOnGrip(local, (size_t) index);
				mPressPoint = event.windowPosition();
				if (mPressedOnGrip) {
					if (ui()) ui()->capturePointer(*this);
					event.accept();
				}
				return;
			}

		case Event::Type::PointerMove:
			{
				if (!mPressed) return;
				const Vec2F offset = event.windowPosition() - mPressPoint;
				const bool moved = std::fabs(offset.x) >= kDragThreshold || std::fabs(offset.y) >= kDragThreshold;
				if (mPressedIndex >= mItems.size()) {
					mPressed = false;
					return;
				}
				if (!mDragging && mHeld && moved) {
					beginDrag(event.windowPosition());
					updateDrag(event.windowPosition());
					event.accept();
					return;
				}
				if (!mDragging && !usesGrips()) return;
				if (!mDragging) {
					const bool vertical = std::fabs(offset.y) >= kDragThreshold && std::fabs(offset.y) >= std::fabs(offset.x);
					if (!vertical) return;
					beginDrag(event.windowPosition());
				}
				updateDrag(event.windowPosition());
				event.accept();
				return;
			}

		case Event::Type::PointerUp:
			if (event.button() != PointerButton::Primary || !mPressed) return;
			mPressed = false;
			if (!mDragging) {
				if (mPressedOnGrip && ui()) ui()->releasePointer(*this);
				if (mHeld && ui()) ui()->releasePointer(*this);
				if (mHeld) {
					mHeld = false;
					event.accept();
					if (mOnHeld && mPressedIndex < mItems.size()) mOnHeld(mPressedIndex);
				}
				return;
			}
			finishDrag();
			event.accept();
			return;

		case Event::Type::LongPress:
			if (mDragMode == DragMode::Grip || !mPressed || mDragging || mPressedIndex >= mItems.size()) return;
			if (mDragMode == DragMode::None && !mOnHeld) return;
			mHeld = true;
			if (ui()) ui()->capturePointer(*this);
			event.accept();
			return;

		case Event::Type::KeyDown:
			if (!mDragging || event.keyCode() != InputID::ESCAPE) return;
			cancelDrag();
			event.accept();
			return;

		default: return;
	}
}

void ReorderableList::beginDrag(Vec2F windowPoint) {
	mDragging = true;
	mHoveredGrip = -1;
	mDragged = mPressedIndex;
	mDrop = mPressedIndex;

	const RectF item = mItems[mDragged]->targetRect();
	mGrabOffset = mPressLocalY - item.pos.y;
	mDragY = item.pos.y;
	mListHeight = size().y;
	mItems[mDragged]->setGeometryAnimated(false);

	if (ui()) {
		ui()->capturePointer(*this);
		mFocusBeforeDrag = ui()->keyboardFocus();
		ui()->setKeyboardFocus(this);
	}
	setCursor(CursorShape::Grabbing);
	requestLayout();
}

void ReorderableList::updateDrag(Vec2F windowPoint) {
	const halnf pointer = toLocal(windowPoint).y;
	const halnf limit = std::max(0.0f, mListHeight - itemHeight(mDragged));
	mDragY = std::clamp(pointer - mGrabOffset, 0.0f, limit);
	mDropTarget = dropTargetAt(pointer);
	mDrop = mDropTarget >= 0 ? mDragged : slotFor(mDragY, mDragY + itemHeight(mDragged));
	requestLayout();
	requestPaint();
}

void ReorderableList::finishDrag() {
	const size_t from = mDragged;
	const size_t to = mDrop;
	const long target = mDropTarget;

	endDragging();
	if (target >= 0) {
		requestLayout();
		if (mOnDroppedOnto) mOnDroppedOnto(from, (size_t) target);
		return;
	}

	Widget* moved = mItems[from];
	mItems.erase(mItems.begin() + (std::ptrdiff_t) from);
	mItems.insert(mItems.begin() + (std::ptrdiff_t) to, moved);
	requestLayout();
	requestPaint();

	if (from != to && mOnMoved) mOnMoved(from, to);
}

void ReorderableList::cancelDrag() {
	if (!mDragging) {
		mPressed = false;
		return;
	}

	mPressed = false;
	endDragging();
	requestLayout();
	requestPaint();
}

void ReorderableList::endDragging() {
	mDragging = false;
	mDropTarget = -1;
	mItems[mDragged]->setGeometryAnimated(true);
	if (ui()) {
		ui()->releasePointer(*this);
		if (ui()->keyboardFocus() == this) ui()->setKeyboardFocus(mFocusBeforeDrag);
	}
	mFocusBeforeDrag = nullptr;
	setCursor(CursorShape::Default);
}

long ReorderableList::itemIndexAt(Vec2F localPoint) const {
	for (size_t index = 0; index < mItems.size(); index++) {
		const RectF rect = mItems[index]->rect();
		if (localPoint.y >= rect.pos.y && localPoint.y < rect.pos.y + rect.size.y + mGap) return (long) index;
	}
	return -1;
}

halnf ReorderableList::itemHeight(size_t index) const { return mItems[index]->targetRect().size.y; }

long ReorderableList::dropTargetAt(halnf pointerY) const {
	if (!mCanAcceptDrop) return -1;
	for (size_t index = 0; index < mItems.size(); index++) {
		if (index == mDragged) continue;
		const RectF rect = mItems[index]->targetRect();
		const halnf inset = rect.size.y * 0.25f;
		const bool overMiddle = pointerY >= rect.pos.y + inset && pointerY <= rect.pos.y + rect.size.y - inset;
		if (overMiddle && mCanAcceptDrop(mDragged, index)) return (long) index;
	}
	return -1;
}

size_t ReorderableList::slotFor(halnf draggedTop, halnf draggedBottom) const {
	size_t slot = mDragged;
	halnf top = 0;
	for (size_t index = 0; index < mItems.size(); index++) {
		const halnf height = itemHeight(index);
		const halnf center = top + height / 2;
		if (index < mDragged && draggedTop < center) slot--;
		if (index > mDragged && draggedBottom > center) slot++;
		top += height + mGap;
	}
	return slot;
}

void ReorderableList::paint(Painter& painter) {
	const Theme& style = theme();

	for (size_t index = 0; index < mItems.size() && usesGrips(); index++) {
		if (mDragging && index == mDragged) continue;
		const RectF grip = gripRect(index);
		const bool highlighted = !mDragging && mHoveredGrip == (long) index;
		if (highlighted) painter.fillRect(grip, style.palette.controlHovered, style.metrics.indicatorCornerRadius);
		paintGrip(painter, grip, highlighted ? style.palette.text : style.palette.disabledText);
	}

	if (!mDragging) return;

	if (mDropTarget >= 0) {
		const RectF target = mItems[(size_t) mDropTarget]->rect();
		painter.strokeRect(target, style.palette.accent, style.metrics.cornerRadius, 3);
		return;
	}

	halnf slotTop = 0;
	size_t slot = 0;
	for (size_t index = 0; index < mItems.size() && slot < mDrop; index++) {
		if (index == mDragged) continue;
		slotTop += itemHeight(index) + mGap;
		slot++;
	}
	const halnf height = itemHeight(mDragged);
	painter.strokeRect(
		makeRect(0, slotTop, size().x - gripWidth(), height), style.palette.focusRing, style.metrics.cornerRadius
	);
}

const Widget* ReorderableList::raisedChild() const { return mDragging ? mItems[mDragged] : nullptr; }

void ReorderableList::paintRaisedBackdrop(Painter& painter, const RectF& childRect) {
	const Theme& style = theme();
	const RectF lifted = makeRect(0, childRect.pos.y, size().x, childRect.size.y);

	painter.fillRect(
		makeRect(lifted.pos.x, lifted.pos.y + kLiftedShadowOffset, lifted.size.x, lifted.size.y),
		style.palette.shadow,
		style.metrics.cornerRadius
	);
	painter.fillRect(lifted, style.palette.raisedSurface, style.metrics.cornerRadius);
	painter.strokeRect(lifted, style.palette.controlBorder, style.metrics.cornerRadius);
	if (usesGrips()) paintGrip(painter, gripRectFor(lifted), style.palette.text);
}

void ReorderableList::paintGrip(Painter& painter, const RectF& row, const RGBA& color) const {
	const Vec2F center = row.center();
	for (int line = -1; line <= 1; line++) {
		for (int column = 0; column < 2; column++) {
			painter.fillCircle(
				{ center.x + ((halnf) column - 0.5f) * kGripDotSpacing, center.y + (halnf) line * kGripDotSpacing },
				kGripDotRadius,
				color
			);
		}
	}
}
