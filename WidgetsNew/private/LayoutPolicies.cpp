#include "LayoutPolicies.hpp"

#include "Widget.hpp"

using namespace tp;

const RectF& WidgetLayout::getArea() {
	return mWidget->getAreaCache();
}

void WidgetLayout::setArea(const RectF& area) {
	mWidget->setAreaCache(area);
}

void WidgetLayout::pickRect() {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = pickRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = pickRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	auto newArea = RectF(rangeX, rangeY);

	setArea(newArea);
}

void WidgetLayout::clampRect() {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = clampRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = clampRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	setArea(RectF(rangeX, rangeY));
}

void WidgetLayout::adjustChildrenRect()  {
	if (mWidget->mChildren.empty()) return;

	switch (mWidget->mLayoutPolicy) {
		case LayoutPolicy::Passive: break;
		case LayoutPolicy::Vertical: adjustLayout(true); break;
		case LayoutPolicy::Horizontal: adjustLayout(false); break;
	}
}


halnf WidgetLayout::changeChildSize(tp::Widget* widget, halnf diff, bool vertical) {
	auto prevSize = widget->getAreaCache().size[vertical];
	{
		auto area = widget->getAreaCache();
		area.size[vertical] += diff;
		widget->setAreaCache(area);
		widget->clampMinMaxSize();
	}
	auto newSize = widget->getAreaCache().size[vertical];

	return newSize - prevSize;
}

void WidgetLayout::adjustLayout(bool vertical) {
	std::vector<std::pair<Widget*, bool>> contributors;
	Vec2F contentSize = { 0, 0 };
	Vec2F availableSize = getRelativeAreaT().size;

	for (auto child : mChildren) {
		if (child->mSizePolicy[!vertical] != SizePolicy::Minimal) {
			child->pickRect();
		}

		if (child->mSizePolicy[vertical] == SizePolicy::Expanding) {
			contributors.emplace_back( child, true );

			auto area = child->getAreaCache();
			area.size[vertical] = 0;
			child->setAreaCache(area);
			child->clampRect();

		}
		contentSize += child->getAreaCache().size;
	}

	availableSize -= mLayoutGap * ((halnf) mChildren.size() - 1) + mLayoutMargin * 2;

	auto diff = availableSize - contentSize;

	// expand or contract as much as possible
	while (!contributors.empty() && (diff[vertical] != 0)) {
		auto quota = diff / (halnf) contributors.size();

		for (auto& contributor : contributors) {
			if (!contributor.second) continue;

			// contributor.first->endAnimations();
			auto contribution = changeChildSize(contributor.first, quota[vertical], vertical);

			if (contribution == 0) {
				contributor.second = false;
			}

			diff[vertical] -= contribution;
		}

		contributors.erase(
			std::remove_if(contributors.begin(), contributors.end(), [](auto node) { return !node.second; }),
			contributors.end()
		);
	}

	// arrange
	halnf iterPos = mLayoutMargin;
	for (auto child : mChildren) {
		auto area = child->getAreaCache();
		area.pos[vertical] = iterPos;
		iterPos += area.size[vertical] + mLayoutGap;
		child->setAreaCache(area);


		// child->updateAnimations();
		// child->triggerWidgetUpdate("layout changed");
	}
}

RangeF WidgetLayout::pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) {
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
	auto current = getAreaCache();
	current.size.clamp(mMinSize, mMaxSize);
	setAreaCache(current);
}

RangeF WidgetLayout::clampRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) {
	auto out = current;

	if (!mChildren.empty()) {
		auto clampedChild = children;
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

	if (mChildren.empty()) {
		out = { getAreaCache().center(), { 0, 0 } };
	} else {
		out = mChildren.front()->getAreaCache();
		for (auto child : mChildren) {
			out.expand(child->getAreaCache());
		}
		out.pos += getAreaCache().pos;
	}

	return out;
}

RectF WidgetLayout::getParentEnclosure() {
	DEBUG_ASSERT(mParent);
	if (!mParent) return { { 0, 0 }, mMaxSize };

	auto out = mParent->getRelativeAreaT();
	if (mParent->mLayoutPolicy != LayoutPolicy::Passive) {
		return out.scaleFromCenter(mParent->mLayoutMargin, true);
	}
	return out;
}