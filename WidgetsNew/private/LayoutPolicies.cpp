#include <algorithm>
#include "LayoutPolicies.hpp"

#include "Widget.hpp"

using namespace tp;

const RectF& WidgetLayout::getArea() const {
	return mWidget->getAreaCache();
}

void WidgetLayout::setArea(const RectF& area) {
	mWidget->setAreaCache(area);
}

Widget* WidgetLayout::parent() const { return mWidget->mParent; }

const std::vector<Widget*>& WidgetLayout::children() const { return mWidget->mChildren; }

const Vec2F& BasicLayout::getMinSize() { return mMinSize; }

void BasicLayout::setMinSize(const Vec2F& size) { mMinSize = size; }

void BasicLayout::pickRect() {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = pickRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = pickRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	auto newArea = RectF(rangeX, rangeY);

	setArea(newArea);
}

void BasicLayout::clampRect() {
	auto current = getArea();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = clampRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = clampRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	setArea(RectF(rangeX, rangeY));
}

void BasicLayout::adjustChildrenRect()  {
	if (children().empty()) return;

	switch (mLayoutPolicy) {
		case LayoutPolicy::Passive: break;
		case LayoutPolicy::Vertical: adjustLayout(true); break;
		case LayoutPolicy::Horizontal: adjustLayout(false); break;
	}
}


halnf BasicLayout::changeChildSize(tp::Widget* widget, halnf diff, bool vertical) {
	auto prevSize = widget->getAreaCache().size[vertical];
	{
		auto area = widget->getAreaCache();
		area.size[vertical] += diff;
		widget->setAreaCache(area);

		// FIXME : widget->clampMinMaxSize();
	}
	auto newSize = widget->getAreaCache().size[vertical];

	return newSize - prevSize;
}

void BasicLayout::adjustLayout(bool vertical) {
	std::vector<std::pair<Widget*, bool>> contributors;
	Vec2F contentSize = { 0, 0 };
	Vec2F availableSize = getArea().size;

	for (auto child : children()) {
		// FIXME :

		// if (child->mSizePolicy[!vertical] != SizePolicy::Minimal) {
		//	child->pickRect();
		// }

		// if (child->mSizePolicy[vertical] == SizePolicy::Expanding) {
			contributors.emplace_back( child, true );

			auto area = child->getAreaCache();
			area.size[vertical] = 0;
			child->setAreaCache(area);
			// child->clampRect();

		//}
		contentSize += child->getAreaCache().size;
	}

	availableSize -= mLayoutGap * ((halnf) children().size() - 1) + mLayoutMargin * 2;

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
	for (auto child : children()) {
		auto area = child->getAreaCache();
		area.pos[vertical] = iterPos;
		iterPos += area.size[vertical] + mLayoutGap;
		child->setAreaCache(area);


		// child->updateAnimations();
		// child->triggerWidgetUpdate("layout changed");
	}
}

RangeF BasicLayout::pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) const {
	RangeF out;

	switch (mSizePolicy[vertical]) {
		case SizePolicy::Fixed: out = current; break;
		case SizePolicy::Expanding: out = parent; break;
		case SizePolicy::Minimal: out = children; break;
	}

	out = clampRange(out, children, parent, vertical);
	return out;
}

void BasicLayout::clampMinMaxSize() {
	auto current = getArea();
	current.size.clamp(mMinSize, mMaxSize);
	setArea(current);
}

RangeF BasicLayout::clampRange(const RangeF& current, const RangeF& child, const RangeF& parent, bool vertical) const {
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

RectF BasicLayout::getChildrenEnclosure() const {
	RectF out;

	if (children().empty()) {
		out = { getArea().center(), { 0, 0 } };
	} else {
		out = children().front()->getAreaCache();
		for (auto child : children()) {
			out.expand(child->getAreaCache());
		}
		out.pos += getArea().pos;
	}

	return out;
}

RectF BasicLayout::getParentEnclosure() const {
	DEBUG_ASSERT(parent())
	if (!parent()) return { { 0, 0 }, mMaxSize };

	// FIXME :
	// auto out = parent()->getRelativeAreaT();

	// if (parent()->mLayoutPolicy != LayoutPolicy::Passive) {
		// return out.scaleFromCenter(parent()->mLayoutMargin, true);
	// }

	// return out;
	return {};
}