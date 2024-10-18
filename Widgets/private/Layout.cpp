#include <algorithm>
#include "Layout.hpp"

#include "Widget.hpp"

using namespace tp;

const RectF& WidgetLayout::getArea() const {
	return mWidget->mAreaCache;
}

RectF WidgetLayout::getAnimatedArea() const {
	return mWidget->getArea();
}

void WidgetLayout::setArea(const RectF& area) {
	mWidget->setAreaCache(area);
}

Widget* WidgetLayout::parent() const { return mWidget->mParent; }

const std::vector<Widget*>& WidgetLayout::children() const { return mWidget->mChildren; }

const Vec2F& WidgetLayout::getMinSize() { return mMinSize; }

void WidgetLayout::setMinSize(const Vec2F& size) { mMinSize = size; }

const Vec2<SizePolicy>& WidgetLayout::getSizePolicy() const { return mSizePolicy; }

void WidgetLayout::setSizePolicy(SizePolicy x, SizePolicy y) { mSizePolicy = { x, y }; }

RangeF WidgetLayout::pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) const {
	RangeF out;

	switch (mSizePolicy[vertical]) {
		case SizePolicy::Fixed: out = current; break;
		case SizePolicy::Expanding: out = parent; break;
		case SizePolicy::Minimal: out = children; break;
	}

	out = clampRange(out, children, parent, vertical);
	return out;
}

void WidgetLayout::clampMinMaxSize() {
	auto current = getArea();
	current.size.clamp(mMinSize, mMaxSize);
	setArea(current);
}

RangeF WidgetLayout::clampRange(const RangeF& current, const RangeF& child, const RangeF& parent, bool vertical) const {
	auto out = current;

	if (!children().empty()) {
		auto clampedChild = child;
		clampedChild.clamp(parent);
		out.clamp(clampedChild, parent);
	} else {
		out.clamp(parent);
	}

	// clamp min max sizes
	auto len = clamp(current.size(), mMinSize[vertical], mMaxSize[vertical]);
	if (len != current.size()) {
		out.resizeFromCenter(len);
	}

	return out;
}

RectF WidgetLayout::getChildrenEnclosure() const {
	RectF out;

	if (children().empty()) {
		out = { getArea().center(), { 0, 0 } };
	} else {
		out = children().front()->getLayout()->getArea();
		for (auto child : children()) {
			out.expand(child->getLayout()->getArea());
		}
		out.pos += getArea().pos;
	}

	return out;
}

RectF WidgetLayout::getAvailableChildArea() const {
	return getArea().relative();
}

RectF WidgetLayout::getParentEnclosure() const {
	DEBUG_ASSERT(parent())
	if (!parent()) return { { 0, 0 }, mMaxSize };
	auto out = parent()->getLayout()->getAvailableChildArea();
	return out;
}