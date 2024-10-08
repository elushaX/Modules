
#include <algorithm>
#include "RootWidget.hpp"

#include "imgui.h"

using namespace tp;

void RootWidget::setRootWidget(Widget* widget) {
	mRoot = widget;
	widget->mParent = this;
}

void RootWidget::processFrame(EventHandler* events, const RectF& screenArea) {
	mScreenArea = screenArea;

	mRoot->setArea(screenArea);

	// construct hierarchy tree of widgets to process
	updateTreeToProcess();

	// update active tree animations
	updateAnimations(&mWidgetsToProcess[mRoot]);

	// update all events and call all event processing callbacks
	EventHandler dummyEvents;

	processFocusItems(*events);
	processActiveTree(&mWidgetsToProcess[mRoot], dummyEvents);

	// update widget sizes base on individual size policies
	adjustSizes(&mWidgetsToProcess[mRoot]);

	// trigger some widgets by moise pointer
	handleFocusChanges(*events);

	// check triggered widgets for removal
	erase_if(mTriggeredWidgets, [](auto widget) {
		auto end = !widget->needsNextFrame();
		if (end) {
			widget->endAnimations();
		}
		return end;
	});
}

bool RootWidget::needsUpdate() const { return !mTriggeredWidgets.empty(); }

void RootWidget::drawFrame(Canvas& canvas) {
	// draw from top to bottom
	canvas.rect(mScreenArea, RGBA(0, 0, 0, 1));

	// draw all tree from top to bottom
	drawRecursion(canvas, mRoot, { 0, 0 });

	if (mDebug) {
		drawDebug(canvas, mRoot, { 0, 0 });
		ImGui::Text("Triggered Widgets: %i", (int) mTriggeredWidgets.size());
		ImGui::Text("Widgets To Process: %i", (int) mWidgetsToProcess.size());
	}
}


void RootWidget::updateTreeToProcess() {
	mWidgetsToProcess.clear();
	for (auto widget : mTriggeredWidgets) {
		for (auto iter = widget; iter; iter = iter->mParent) {
			if (iter->mParent) {
				mWidgetsToProcess[iter->mParent].children.insert(&mWidgetsToProcess[iter]);
				mWidgetsToProcess[iter->mParent].widget= iter->mParent;
				mWidgetsToProcess[iter].widget= iter;
			}
		}
	}
}

void RootWidget::drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos) {
	canvas.setOrigin(pos);
	canvas.pushClamp({ pos, active->getArea().size });

	active->draw(canvas);

	for (auto child : active->mChildren) {
		drawRecursion(canvas, child, pos + child->getArea().pos);
	}

	canvas.setOrigin(pos);
	canvas.popClamp();

	active->drawOverlay(canvas);
}

void RootWidget::drawDebug(Canvas& canvas, Widget* active, const Vec2F& pos) {
	auto area = RectF{ pos, active->getArea().size };


	if (mWidgetsToProcess.find(active) != mWidgetsToProcess.end()) {
		RGBA color = { 1, 0, 0, 1};
		canvas.frame(area, color);
		canvas.text(active->mName.c_str(), area, 12, Canvas::Align::LC, 2, color);
	}

	if (active->mInFocus) {
		RGBA color = { 1, 0, 0, 0.3f};
		canvas.debugCross(area, color);
		canvas.circle(pos + active->mLocalPoint, 5, active->mDebugColor);
		canvas.circle(active->mGlobalPoint, 10, active->mDebugColor);
	}

	for (auto child : active->mChildren) {
		drawDebug(canvas, child, pos + child->getArea().pos);
	}
}

void RootWidget::setWidgetArea(Widget& widget, const RectF& rect) {
	widget.mArea.setTargetRect(rect);
	widget.mArea.endAnimation();
}

void RootWidget::updateAnimations(ActiveTreeNode* iter) {
	if (!iter) return;
	iter->widget->updateAnimations();
	for (auto child : iter->children) {
		updateAnimations(child);
	}
}

void RootWidget::getWidgetPath(Widget* widget, std::vector<Widget*>& out) {
	for (auto iter = widget; iter && iter != this; iter = iter->mParent) {
		out.push_back(iter);
	}
}

void RootWidget::handleFocusChanges(EventHandler& events) {
	auto prevFocus = mInFocusWidget;
	findFocusWidget(mRoot, &mInFocusWidget, events.getPointer());

	if (prevFocus) {
		std::vector<Widget*> path1;
		std::vector<Widget*> path2;

		getWidgetPath(prevFocus, path1);
		getWidgetPath(mInFocusWidget, path2);

		size_t iter = 0;
		while (path1[iter] == path2[iter]) {
			iter++;
		}

		for (auto i = iter; i < path1.size(); i++) {
			path1[i]->mouseLeave();
		}

		for (auto i = iter; i < path2.size(); i++) {
			path2[i]->mouseEnter();
		}

	} else {
		for (auto iter = mInFocusWidget; iter; iter = iter->mParent) {
			iter->mouseEnter();
		}
	}
}

void RootWidget::findFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer) {
	if (!iter->mArea.getTargetRect().isInside(pointer)) return;

	*focus = iter;

	for (auto child : iter->mChildren) {
		findFocusWidget(child, focus, pointer - iter->mArea.getTargetRect().pos);
	}
}

void RootWidget::updateWidget(Widget* widget) {
	mTriggeredWidgets.insert(widget);
}

void RootWidget::adjustSizes(ActiveTreeNode* iter) {
	if (!iter) return;

	iter->widget->adjustRect();
	iter->widget->adjustChildrenRect();

	for (auto child : iter->children) {
		adjustSizes(child);
	}
}

void RootWidget::processActiveTree(ActiveTreeNode* iter, EventHandler& events) {
	if (!iter) return;

	if (!iter->widget->mInFocus) {
		iter->widget->updateAnimations();
		iter->widget->process(events);
	}

	for (auto child : iter->children) {
		 processActiveTree(child, events);
	}
}

void RootWidget::processFocusItems(EventHandler& events) {
	if (!mInFocusWidget) return;

	// FIXME : cringe

	std::vector<Widget*> path;
	getWidgetPath(mInFocusWidget, path);

	for (auto i = 0; i < path.size() / 2; i++) {
		swap(path[i], path[path.size() - i - 1]);
	}

	size_t len = path.size();
	for (auto i = 0; i < len; i++) {
		if (!path[i]->propagateEventsToChildren()) {
			len = i + 1;
			break;
		}
	}

	std::vector<Vec2F> widgetGlobalPos(path.size());

	widgetGlobalPos[0] = 0;
	for (auto widget = 0; widget < path.size() - 1; widget++) {
		widgetGlobalPos[widget + 1] = widgetGlobalPos[widget] + path[widget + 1]->getArea().pos;
		path[widget]->mGlobalPoint = widgetGlobalPos[widget];
	}

	bool eventsProcessed = false;

	for (int iter = (int) len - 1; iter >= 0; iter--) {
		auto widget = path[iter];

		events.setCursorOrigin(widgetGlobalPos[iter]);

		if (!eventsProcessed && widget->processesEvents()) {
			events.setEnableKeyEvents(true);

			widget->updateAnimations();
			widget->process(events);

			events.setEnableKeyEvents(false);
			eventsProcessed = true;
		} else {
			widget->updateAnimations();
			widget->process(events);
		}
	}
}
