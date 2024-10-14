
#include "RootWidget.hpp"
#include "BasicLayout.hpp"


#include <algorithm>

using namespace tp;

WidgetLayout* WidgetManagerInterface::defaultLayout(Widget* widget) {
	return new BasicLayout(widget);
}

RootWidget::RootWidget() {
	setDebug("root", RGBA(1));
}

void RootWidget::setRootWidget(Widget* widget) {
	mRoot = widget;
	widget->mParent = this;
}

void RootWidget::processFrame(EventHandler* events, const RectF& screenArea) {
	mRoot->setArea(screenArea);

	if (!gDebugWidget.update(this, *events)) return;

	// construct hierarchy tree of widgets to process
	mUpdateManager.updateTreeToProcess(mRoot);

	updateAnimations();
	updateAreaCache(mRoot, true);

	// update all events and call all event processing callbacks
	mUpdateManager.processWidgets(mRoot, *events);

	updateAreaCache(mRoot, true);

	// update widget sizes base on individual size policies
	mLayoutManager.adjust(mRoot);

	updateAreaCache(mRoot, false);

	// trigger some widgets by moise pointer
  mUpdateManager.handleFocusChanges(mRoot, *events);

	// check triggered widgets for removal
	mUpdateManager.clean();
}

bool RootWidget::needsUpdate() const {
	if (gDebugWidget.isRedrawAlways()) return true;
	return mUpdateManager.isPendingUpdates();
}

void RootWidget::drawFrame(Canvas& canvas) {
	canvas.rect(mRoot->getAreaT(), RGBA(0, 0, 0, 1));

	drawRecursion(canvas, mRoot, { 0, 0 });

	gDebugWidget.drawDebug(this, canvas);
}


void RootWidget::drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos) {
	if (!active->mFlags.get(ENABLED)) return;

	canvas.setOrigin(pos);
	canvas.pushClamp({ pos, active->getArea().size });

	active->draw(canvas);

	for (auto child = active->mDepthOrder.lastNode(); child; child = child->prev) {
		drawRecursion(canvas, child->data, pos + child->data->getArea().pos);
	}

	canvas.setOrigin(pos);
	canvas.popClamp();

	active->drawOverlay(canvas);
}

void RootWidget::setWidgetArea(Widget& widget, const RectF& rect) {
	widget.mArea.setTargetRect(rect);
	widget.mArea.endAnimation();
}

void RootWidget::updateAnimations() {
	dfs(mRoot, [](Widget* widget) {
		widget->updateAnimations();
	});
}


void RootWidget::updateWidget(Widget* widget, const char* reason) {
	DEBUG_ASSERT(reason)
	mUpdateManager.scheduleUpdate(widget, reason);
}

void RootWidget::updateAreaCache(Widget* iter, bool read) {
	if (!iter || !iter->isUpdate()) return;

	if (read) {
		iter->mAreaCache = iter->getAreaT();
	} else {
		iter->setArea(iter->mAreaCache);
		iter->mArea.updateCurrentRect();
	}

	for (auto child : iter->mChildren) {
		if (read) {
			child->mAreaCache = child->getAreaT();
		} else {
			child->setArea(child->mAreaCache);
			child->mArea.updateCurrentRect();
		}
	}

	for (auto child : iter->mDepthOrder) {
		updateAreaCache(child.data(), read);
	}
}