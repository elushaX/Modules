#include "Widget.hpp"

#include <algorithm>

using namespace tp;

Widget::Widget() {
	mArea.setTargetRect({ 100, 100, 10, 10 });
	setLayout(WidgetManagerInterface::defaultLayout(this));
	mArea.endAnimation();

	mFlags.set(ENABLED, true);
}

Widget::~Widget() {
	delete mLayout;
}

void Widget::addChild(Widget* child, bool front) {
	if (auto node = mDepthOrder.find(child)) {
		node->data->bringToFront();
		return;
	}

	if (child->mParent) {
		child->mParent->removeChild(child);
	}

	mChildren.push_back(child);

	if (front) {
		mDepthOrder.pushFront(child);
	} else {
		mDepthOrder.pushBack(child);
	}

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

void Widget::endAnimations() { mArea.endAnimation(); }
bool Widget::processesEvents() const { return false; }
void Widget::updateAnimations() { mArea.updateCurrentRect(); }
bool Widget::needsNextFrame() const { return !mArea.shouldEndTransition(); }

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

void Widget::mouseEnter() { mFlags.set(IN_FOCUS, true); }
void Widget::mouseLeave() { mFlags.set(IN_FOCUS, false); }

bool Widget::propagateEventsToChildren() const { return true; }

void Widget::setLayout(tp::WidgetLayout* layout) {
	delete mLayout;
	mLayout = layout;
	triggerWidgetUpdate("chane layout");
}

WidgetLayout* Widget::getLayout() { return mLayout; }

const WidgetLayout* Widget::getLayout() const { return mLayout; }

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

void Widget::setAreaCache(const tp::RectF& area) { mAreaCache = area; }

void Widget::setArea(const RectF& area) {
	if (mArea.getTargetRect() == area) return;

	mArea.setTargetRect(area);
	triggerWidgetUpdate("new area");
}

RectF Widget::getArea() const { return mArea.getCurrentRect(); }

RectF Widget::getAreaT() const { return mArea.getTargetRect(); }

RectF Widget::getRelativeArea() const { return { {}, mArea.getCurrentRect().size }; }

RectF Widget::getRelativeAreaT() const { return { {}, mArea.getTargetRect().size }; }

void Widget::setDebug(const char* name, RGBA col) {
	mDebug.id = name;
	mDebug.col = col;
}

void Widget::setSizePolicy(SizePolicy x, SizePolicy y) {
	getLayout()->setSizePolicy(x, y);
}
