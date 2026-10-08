#include "Layout.hpp"

#include "LayoutEngine.hpp"
#include "Widget.hpp"

#include <vector>

using namespace tp;
using namespace tp::ui;

LayoutContext::LayoutContext(LayoutEngine& engine, Widget& owner) :
	mEngine(engine),
	mOwner(owner) {
	for (Widget* child : owner.children()) {
		if (child->isVisible()) mVisibleChildren.push_back(child);
	}
}

LayoutContext LayoutContext::without(const Widget& child) const {
	LayoutContext context = *this;
	std::erase(context.mVisibleChildren, &child);
	return context;
}

Extent LayoutContext::measure(const Widget& child, Axis axis, halnf crossLength) const {
	return mEngine.measure(child, axis, crossLength);
}

void LayoutContext::place(Widget& child, const RectF& rect) const { mEngine.place(child, rect); }

void Layout::setPadding(const Insets& padding) {
	mPadding = padding;
	invalidate();
}

Extent Layout::measure(const LayoutContext& context, Axis axis, halnf crossLength) const {
	const halnf innerCross = crossLength >= kUnbounded ? kUnbounded : crossLength - mPadding.total(crossAxis(axis));
	Extent extent = measureChildren(context, axis, innerCross > 0 ? innerCross : 0);

	const halnf padding = mPadding.total(axis);
	extent.minimum += padding;
	extent.preferred += padding;
	if (extent.maximum < kUnbounded) extent.maximum += padding;
	return extent;
}

void Layout::arrange(LayoutContext& context, const RectF& ownerArea) {
	arrangeChildren(context, mPadding.shrink(ownerArea));
}

void Layout::invalidate() {
	if (mOwner) mOwner->requestLayout();
}
