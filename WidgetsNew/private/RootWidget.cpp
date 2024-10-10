
#include <algorithm>
#include "RootWidget.hpp"

#include "imgui.h"

using namespace tp;

void RootWidget::setRootWidget(Widget* widget) {
	mRoot = widget;
	widget->mParent = this;
}

void RootWidget::processFrame(EventHandler* events, const RectF& screenArea) {
	if (mDebug) {
		events->setEnableKeyEvents(true);
		if (events->isPressed(InputID::K)) mDebugStopProcessing = !mDebugStopProcessing;
		if (mDebugStopProcessing) return;
	}

	mScreenArea = screenArea;

	mRoot->setArea(screenArea);

	// construct hierarchy tree of widgets to process
	updateTreeToProcess();

	auto root = mWidgetsToProcess.find(mRoot) == mWidgetsToProcess.end() ? nullptr : &mWidgetsToProcess[mRoot];

	// update active tree animations
	updateAnimations(root);

	// update all events and call all event processing callbacks
	EventHandler dummyEvents;

	processFocusItems(*events);
	processActiveTree(root, dummyEvents);

	// update widget sizes base on individual size policies
	adjustSizes(root);

	// trigger some widgets by moise pointer
  handleFocusChanges(*events);

		// check triggered widgets for removal
	erase_if(mTriggeredWidgets, [this](auto widget) {
		widget->updateAnimations();
		if (mWidgetsToProcess.find(widget) == mWidgetsToProcess.end()) return false;
		auto end = !widget->needsNextFrame();
		if (end) {
			widget->endAnimations();
		}
		return end;
	});
}

bool RootWidget::needsUpdate() const { return !mTriggeredWidgets.empty() || mDebugRedrawAlways; }

void RootWidget::drawFrame(Canvas& canvas) {
	// draw from top to bottom
	canvas.rect(mScreenArea, RGBA(0, 0, 0, 1));

	// draw all tree from top to bottom
	drawRecursion(canvas, mRoot, { 0, 0 });

	ImGui::Checkbox("Draw debug", &mDebug);

	if (mDebug) {
		drawDebug(canvas, mRoot, { 0, 0 }, 0);

		ImGui::Text("Triggered Widgets: %i", (int) mTriggeredWidgets.size());
		ImGui::Text("Widgets To Process: %i", (int) mWidgetsToProcess.size());

		ImGui::Text("To Toggle processing press k");
		ImGui::Checkbox("Stop processing", &mDebugStopProcessing);
		ImGui::Checkbox("Force new frames", &mDebugRedrawAlways);


		if (mInFocusWidget) {
			ImGui::Text("Item under cursor %s", mInFocusWidget->mName.c_str());

			if (ImGui::BeginListBox("##empty", { -FLT_MIN, 0 })) {
				for (auto widget = mInFocusWidget; widget && widget != this; widget = widget->mParent) {
					if (ImGui::CollapsingHeader(widget->mName.c_str())) {
						ImGui::InputFloat2("min size", &widget->mMinSize.x);
						ImGui::InputFloat2("max size", &widget->mMaxSize.x);

						int sizePolicyX = int(widget->mSizePolicy.x);
						int sizePolicyY = int(widget->mSizePolicy.y);
						int layout = int(widget->mLayoutPolicy);

						if (ImGui::Combo("Size Policy X", &sizePolicyX, "Passive\0Expand\0Contract\0")) {
							widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
						}

						if (ImGui::Combo("Size Policy Y", &sizePolicyY, "Passive\0Expand\0Contract\0")) {
							widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
						}

						if (ImGui::Combo("Layout", &layout, "Passive\0Vertical\0Horizontal\0")) {
							widget->setLayoutPolicy(LayoutPolicy(layout));
						}
					}
				}
				ImGui::EndListBox();
			}
		}
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

	for (auto child = active->mDepthOrder.lastNode(); child; child = child->prev) {
		drawRecursion(canvas, child->data, pos + child->data->getArea().pos);
	}

	canvas.setOrigin(pos);
	canvas.popClamp();

	active->drawOverlay(canvas);
}

void RootWidget::drawDebug(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder) {
	auto area = RectF{ pos, active->getArea().size };

	if (mWidgetsToProcess.find(active) != mWidgetsToProcess.end()) {
		RGBA color = { 1, 0, 0, 1};
		canvas.frame(area, color);
		canvas.text((active->mName + ":" + std::to_string(depthOrder)).c_str(), area, 22, Canvas::Align::LC, 2, color);
	}

	if (active->mInFocus) {
		RGBA color = { 1, 0, 0, 0.3f};
		canvas.debugCross(area, color);
		canvas.circle(pos + active->mLocalPoint, 5, active->mDebugColor);
		canvas.circle(active->mGlobalPoint, 10, active->mDebugColor);
	}

	int orderIdx = 0;
	for (auto child = active->mDepthOrder.lastNode(); child; child = child->prev) {
		drawDebug(canvas, child->data, pos + child->data->getArea().pos, orderIdx++);
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
	if (!widget) return;

	for (auto iter = widget; iter && iter != this; iter = iter->mParent) {
		out.push_back(iter);
	}

	for (auto i = 0; i < out.size() / 2; i++) {
		swap(out[i], out[out.size() - i - 1]);
	}
}

void RootWidget::handleFocusChanges(EventHandler& events) {
	auto prevFocus = mInFocusWidget;

	findFocusWidget(mRoot, &mInFocusWidget, events.getPointer());

	if (mInFocusWidget) updateWidget(mInFocusWidget);

	std::vector<Widget*> path2;
	getWidgetPath(mInFocusWidget, path2);
	size_t propLen2 = path2.size();
	for (auto i = 0; i < path2.size(); i++) {
		if (!path2[i]->propagateEventsToChildren()) {
			propLen2 = i + 1;
			break;
		}
	}

	std::vector<Widget*> path1;
	getWidgetPath(prevFocus, path1);
	size_t propLen1 = path1.size();
	for (auto i = 0; i < path1.size(); i++) {
		if (!path1[i]->propagateEventsToChildren()) {
			propLen1 = i + 1;
			break;
		}
	}

	size_t mostCommonIdx = 0;

	if (!(path1.empty() || path2.empty())) {
		while (path1[mostCommonIdx] == path2[mostCommonIdx]) {
			mostCommonIdx++;
		}
	}

	for (auto i = 0; i < path1.size(); i++) {
		path1[i]->mInFocus = false;
		if (i >= mostCommonIdx && i < propLen1) path1[i]->mouseLeave();
	}

	for (auto i = 0; i < path2.size(); i++) {
		path2[i]->mInFocus = true;
		if (i >= mostCommonIdx && i < propLen2) path2[i]->mouseEnter();
	}
}

void RootWidget::findFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer) {
	if (!iter->mArea.getTargetRect().isInside(pointer)) return;

	*focus = iter;

	for (auto child = iter->mDepthOrder.lastNode(); child; child = child->prev) {
		findFocusWidget(child->data, focus, pointer - iter->mArea.getTargetRect().pos);
	}
}

void RootWidget::updateWidget(Widget* widget) {
	mTriggeredWidgets.insert(widget);
}

void RootWidget::adjustSizes(ActiveTreeNode* iter) {
	if (!iter) return;

	for (auto child : iter->children) {
		adjustSizes(child);
	}

	iter->widget->pickRect();
	iter->widget->adjustChildrenRect();
	// iter->widget->pickRect();
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

		widget->mLocalPoint = events.getPointer();

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
