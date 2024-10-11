#include "Widget.hpp"

#include <algorithm>

using namespace tp;

WidgetManagerInterface* Widget::getRoot() {
	Widget* iter = mParent;
	while (iter && iter->mParent) {
		iter = iter->mParent;
	}
	return dynamic_cast<WidgetManagerInterface*>(iter);
}

void Widget::triggerWidgetUpdate(const char* reason) {
	if (auto root = getRoot()) {
		root->updateWidget(this, reason);
	}
}

Widget::Widget() {
	mArea.setTargetRect({ 100, 100, 10, 10, });
	mArea.endAnimation();
}

void Widget::setAreaCache(const tp::RectF& area) {
	mAreaCache = area;
}

void Widget::setArea(const RectF& area) {
	if (mArea.getTargetRect() == area) return;

	mArea.setTargetRect(area);
	triggerWidgetUpdate("new area");
}

RectF Widget::getArea() const {
	return mArea.getCurrentRect();
}

RectF Widget::getAreaT() const {
	return mArea.getTargetRect();
}

RectF Widget::getAreaCache() const {
	return mAreaCache;
}

RectF Widget::getRelativeArea() const {
	return { {}, mArea.getCurrentRect().size };
}

RectF Widget::getRelativeAreaT() const {
	return { {}, mArea.getTargetRect().size };
}

RectF Widget::getRelativeAreaCache() const {
	return { {}, mAreaCache.size };
}

void Widget::endAnimations() {
	mArea.endAnimation();
}

bool Widget::processesEvents() const {
	return false;
}

void Widget::updateAnimations() {
	mArea.updateCurrentRect();
}

bool Widget::needsNextFrame() const {
	return !mArea.shouldEndTransition(); // || mInFocus;
}

void Widget::bringToFront() {
	if (!mParent) return;
	auto& order = mParent->mDepthOrder;
	auto node = order.find(this);
	DEBUG_ASSERT(node)
	order.detach(node);
	order.pushFront(node);
}

void Widget::bringToBack() {
	if (!mParent) return;
	auto& order = mParent->mDepthOrder;
	auto node = order.find(this);
	DEBUG_ASSERT(node)
	order.detach(node);
	order.pushBack(node);
}

void Widget::mouseEnter() {
	mInFocus = true;
}

void Widget::mouseLeave() {
	mInFocus = false;
}

bool Widget::propagateEventsToChildren() const {
	return true;
}

void Widget::addChild(Widget* child) {
	if (auto node = mDepthOrder.find(child)) {
		node->data->bringToFront();
		return;
	}

	if (child->mParent) {
		child->mParent->removeChild(child);
	}

	mChildren.push_back(child);
	mDepthOrder.pushBack(child);

	child->mParent = this;

	triggerWidgetUpdate("add child");
	child->triggerWidgetUpdate("new parent");
}

void Widget::removeChild(Widget* child) {
	auto node = mDepthOrder.find(child);
	if (!node) return;

	mDepthOrder.removeNode(node);
	mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), child), mChildren.end());

	triggerWidgetUpdate("removed child");
	child->triggerWidgetUpdate("parent changed");
}

void Widget::setSizePolicy(tp::Widget::SizePolicy x, tp::Widget::SizePolicy y) {
	mSizePolicy = { x, y };
	triggerWidgetUpdate("chane size policy");
}

void Widget::setLayoutPolicy(LayoutPolicy layoutPolicy) {
	mLayoutPolicy = layoutPolicy;
	triggerWidgetUpdate("new layout");
}

const Vec2F& Widget::getMinSize() { return mMinSize; }

void Widget::setMinSize(const Vec2F& size) {
	mMinSize = size;
	triggerWidgetUpdate("new min size");
}

void Widget::pickRect() {
	auto current = getAreaCache();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = pickRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = pickRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	auto newArea = RectF(rangeX, rangeY);

	for (auto child :  mChildren) {
		child->setAreaCache(child->getAreaCache().move(current.pos - newArea.pos));
	}

	setAreaCache(newArea);
}

void Widget::clampRect() {
	auto current = getAreaCache();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = clampRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = clampRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	setAreaCache(RectF(rangeX, rangeY));
}

void Widget::adjustChildrenRect()  {
	if (mChildren.empty()) return;

	switch (mLayoutPolicy) {
		case LayoutPolicy::Passive: break;
		case LayoutPolicy::Vertically: adjustLayout(true); break;
		case LayoutPolicy::Horizontally: adjustLayout(false); break;
	}
}


halnf Widget::changeChildSize(tp::Widget* widget, halnf diff, bool vertical) {
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

void Widget::adjustLayout(bool vertical) {
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

RangeF Widget::pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) {
	RangeF out;

	switch (mSizePolicy[vertical]) {
		case SizePolicy::Fixed: out = current; break;
		case SizePolicy::Expanding: out = parent; break;
		case SizePolicy::Minimal: out = children; break;
	}

	out = clampRange(out, children, parent, vertical);
	return out;
}

void Widget::clampMinMaxSize() {
	auto current = getAreaCache();
	current.size.clamp(mMinSize, mMaxSize);
	setAreaCache(current);
}

RangeF Widget::clampRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) {
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

RectF Widget::getChildrenEnclosure() {
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

RectF Widget::getParentEnclosure() {
	DEBUG_ASSERT(mParent);
	if (!mParent) return { { 0, 0 }, mMaxSize };

	auto out = mParent->getRelativeAreaT();
	if (mParent->mLayoutPolicy != LayoutPolicy::Passive) {
		return out.scaleFromCenter(mParent->mLayoutMargin, true);
	}
	return out;
}
