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

void Widget::triggerWidgetUpdate() {
	if (auto root = getRoot()) {
		root->updateWidget(this);
	}
}

Widget::Widget() {
	mArea.setTargetRect({ 100, 100, 10, 10, });
	mArea.endAnimation();
}

void Widget::setArea(const RectF& area) {
	if (mArea.getTargetRect() == area) return;

	mArea.setTargetRect(area);
	triggerWidgetUpdate();
}

RectF Widget::getArea() const {
	return mArea.getCurrentRect();
}

RectF Widget::getAreaT() const {
	return mArea.getTargetRect();
}

RectF Widget::getRelativeArea() const {
	return { {}, mArea.getCurrentRect().size };
}

RectF Widget::getRelativeAreaT() const {
	return { {}, mArea.getTargetRect().size };
}

void Widget::endAnimations() {
	mArea.endAnimation();
}

bool Widget::processesEvents() const {
	return true;
}

void Widget::updateAnimations() {
	mArea.updateCurrentRect();
}

bool Widget::needsNextFrame() const {
	return !mArea.shouldEndTransition();
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

	triggerWidgetUpdate();
	child->triggerWidgetUpdate();
}

void Widget::removeChild(Widget* child) {
	auto node = mDepthOrder.find(child);
	if (!node) return;

	mDepthOrder.removeNode(node);
	mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), child), mChildren.end());

	triggerWidgetUpdate();
	child->triggerWidgetUpdate();
}

void Widget::setSizePolicy(tp::Widget::SizePolicy x, tp::Widget::SizePolicy y) {
	mSizePolicy = { x, y };
	triggerWidgetUpdate();
}

void Widget::setLayoutPolicy(LayoutPolicy layoutPolicy) {
	mLayoutPolicy = layoutPolicy;
	triggerWidgetUpdate();
}

const Vec2F& Widget::getMinSize() { return mMinSize; }

void Widget::setMinSize(const Vec2F& size) {
	mMinSize = size;
	triggerWidgetUpdate();
}

void Widget::pickRect() {
	auto current = getAreaT();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = pickRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = pickRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	auto newArea = RectF(rangeX, rangeY);

	for (auto child :  mChildren) {
		child->setArea(child->getAreaT().move(current.pos - newArea.pos));
	}

	setArea(newArea);
}

void Widget::clampRect() {
	auto current = getAreaT();
	auto children = getChildrenEnclosure();
	auto parent = getParentEnclosure();

	auto rangeX = clampRange(current.getRangeX(), children.getRangeX(), parent.getRangeX(), false);
	auto rangeY = clampRange(current.getRangeY(), children.getRangeY(), parent.getRangeY(), true);

	setArea(RectF(rangeX, rangeY));
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
	auto prevSize = widget->getAreaT().size[vertical];
	{
		auto area = widget->getAreaT();
		area.size[vertical] += diff;
		widget->setArea(area);
		widget->clampRect();
	}
	auto newSize = widget->getAreaT().size[vertical];

	return newSize - prevSize;
}

void Widget::adjustLayout(bool vertical) {
	std::vector<std::pair<Widget*, bool>> contributors;
	Vec2F contentSize = { 0, 0 };
	Vec2F availableSize = getRelativeAreaT().size;

	for (auto child : mChildren) {
		contentSize += child->getAreaT().size;
		if (child->mSizePolicy[vertical] == SizePolicy::Expand) {
			contributors.emplace_back( child, true );
		}
	}

	auto diff = availableSize - contentSize;

	// expand or contract as much as possible
	while (!contributors.empty() && (diff[vertical] != 0)) {
		auto quota = diff / (halnf) contributors.size();

		for (auto& contributor : contributors) {
			if (!contributor.second) continue;

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
	halnf iterPos = 0;
	for (auto child : mChildren) {
		auto area = child->getAreaT();
		area.pos[vertical] = iterPos;
		iterPos += area.size[vertical];
		child->setArea(area);
	}
}

RangeF Widget::pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical) {
	RangeF out;

	switch (mSizePolicy[vertical]) {
		case SizePolicy::Passive: out = current; break;
		case SizePolicy::Expand: out = parent; break;
		case SizePolicy::Contract: out = children; break;
	}

	out = clampRange(out, children, parent, vertical);
	return out;
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
		out = { getAreaT().center(), { 0, 0 } };
	} else {
		for (auto child : mChildren) {
			out.expand(child->getAreaT());
		}
		out.pos += getAreaT().pos;
	}

	return out;
}

RectF Widget::getParentEnclosure() {
	DEBUG_ASSERT(mParent);
	if (!mParent) return { { 0, 0 }, mMaxSize };
	return mParent->getRelativeAreaT();
}
