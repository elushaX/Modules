
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

	if (mDebug) {
		mScreenArea.size -= { 400, 0 };
		if (events->isPressed(InputID::K)) mDebugStopProcessing = !mDebugStopProcessing;
		if (mDebugStopProcessing) return;
	}

	mRoot->setArea(mScreenArea);

	// construct hierarchy tree of widgets to process
	updateTreeToProcess();

	auto root = mWidgetsToProcess.find(mRoot) == mWidgetsToProcess.end() ? nullptr : &mWidgetsToProcess[mRoot];

	// update active tree animations
	updateAnimations(root);

	// update all events and call all event processing callbacks
	events->setEnableKeyEvents(true);
	events->setCursorOrigin({ 0, 0 });
	processFocusItems(*events);

	events->setEnableKeyEvents(false);
	events->setCursorOrigin({ 0, 0 });
	processActiveTree(root, *events, { 0, 0 });

	updateAreaCache(root, true);

	// update widget sizes base on individual size policies
	adjustSizes(root);

	updateAreaCache(root, false);

	// trigger some widgets by moise pointer
  handleFocusChanges(*events);

	// check triggered widgets for removal
	erase_if(mTriggeredWidgets, [](auto iter) {
		auto widget = iter.first;
		auto flag = iter.second;

		if (!flag) return false;

		// if (mWidgetsToProcess.find(widget) == mWidgetsToProcess.end()) return false;
		widget->updateAnimations();
		auto end = !widget->needsNextFrame();
		if (end) {
			widget->endAnimations();
			widget->mTriggerReason = "del";
		}
		return end;
	});
}

bool RootWidget::needsUpdate() const { return !mTriggeredWidgets.empty() || mDebugRedrawAlways; }

void RootWidget::debugDrawWidget(Widget* widget) {

	ImGui::PushID(widget);
	if (ImGui::CollapsingHeader(widget->mName.c_str())) {

		ImGui::Text("trigger reason: %s", widget->mTriggerReason.c_str());

		auto area = widget->getAreaT();
		if (ImGui::InputFloat4("rect", &area.x)) {
			widget->setArea(area);
		}

		ImGui::InputFloat2("min size", &widget->mMinSize.x);
		ImGui::InputFloat2("max size", &widget->mMaxSize.x);

		int sizePolicyX = int(widget->mSizePolicy.x);
		int sizePolicyY = int(widget->mSizePolicy.y);
		int layout = int(widget->mLayoutPolicy);

		if (ImGui::Combo("Size Policy X", &sizePolicyX, "Fixed\0Expanding\0Minimal\0")) {
			widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
		}

		if (ImGui::Combo("Size Policy Y", &sizePolicyY, "Fixed\0Expanding\0Minimal\0")) {
			widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
		}

		if (ImGui::Combo("Layout", &layout, "Passive\0Vertical\0Horizontal\0")) {
			widget->setLayoutPolicy(LayoutPolicy(layout));
		}
	}
	ImGui::PopID();
}

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

		if (ImGui::CollapsingHeader("Triggered")) {
			if (ImGui::BeginListBox("##triggered", { -FLT_MIN, 300 })) {
				for (auto widget : mTriggeredWidgets) {
					debugDrawWidget(widget.first);
				}
				ImGui::EndListBox();
			}
		}

		if (mInFocusWidget) {
			if (ImGui::CollapsingHeader("Under cursor")) {
				if (ImGui::BeginListBox("##under_cursor", { -FLT_MIN, 300 })) {
					for (auto widget = mInFocusWidget; widget && widget != this; widget = widget->mParent) {
						debugDrawWidget(widget);
					}
					ImGui::EndListBox();
				}
			}
		}
	}
}


void RootWidget::updateTreeToProcess() {
	mWidgetsToProcess.clear();
	for (auto& [widget, flag] : mTriggeredWidgets) {
		flag = true;
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
	auto area = RectF{ pos, active->getAreaT().size };

	auto processed = mWidgetsToProcess.find(active) != mWidgetsToProcess.end();

	if (processed) {
		RGBA color = { 1, 0, 0, 1};
		canvas.frame(area, color);
		canvas.text((active->mName + ":" + std::to_string(depthOrder)).c_str(), area, 22, Canvas::Align::LC, 2, color);
	}

	if (processed || active->mInFocus) {
		canvas.circle(pos + active->mLocalPoint, 5, active->mDebugColor);
		canvas.circle(active->mGlobalPoint, 15, active->mDebugColor);
	}

	if (active->mInFocus) {
		RGBA color = { 1, 0, 0, 0.3f};
		canvas.debugCross(area, color);
	}

	int orderIdx = 0;
	for (auto child = active->mDepthOrder.lastNode(); child; child = child->prev) {
		drawDebug(canvas, child->data, pos + child->data->getAreaT().pos, orderIdx++);
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

	events.setCursorOrigin({ 0, 0 });

	findFocusWidget(mRoot, &mInFocusWidget, events.getPointer());

	// if (mInFocusWidget == prevFocus) return;
	if (mInFocusWidget) updateWidget(mInFocusWidget, "focus entered");

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
		while (path1[mostCommonIdx] == path2[mostCommonIdx] && mostCommonIdx < min(path1.size(), path2.size())) {
			mostCommonIdx++;
		}
	}
	mostCommonIdx--;

	for (auto i = 0; i < path1.size(); i++) {
		path1[i]->mInFocus = false;
		if (i > mostCommonIdx && i < propLen1) path1[i]->mouseLeave();
	}

	for (auto i = 0; i < path2.size(); i++) {
		path2[i]->mInFocus = true;
		if (i > mostCommonIdx && i < propLen2) path2[i]->mouseEnter();
	}
}

void RootWidget::findFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer) {
	if (!iter->mArea.getTargetRect().isInside(pointer)) return;

	*focus = iter;

	for (auto child = iter->mDepthOrder.lastNode(); child; child = child->prev) {
		findFocusWidget(child->data, focus, pointer - iter->mArea.getTargetRect().pos);
	}
}

void RootWidget::updateWidget(Widget* widget, const char* reason) {
	DEBUG_ASSERT(reason)
	widget->mTriggerReason = reason;
	mTriggeredWidgets.insert({ widget, false });
}

void RootWidget::updateAreaCache(ActiveTreeNode* iter, bool read) {
	if (!iter) return;

	if (read) {
		iter->widget->mAreaCache = iter->widget->getAreaT();
	} else {
		iter->widget->setArea(iter->widget->mAreaCache);
		iter->widget->mArea.updateCurrentRect();
	}

	for (auto child : iter->widget->mChildren) {
		if (read) {
			child->mAreaCache = child->getAreaT();
		} else {
			child->setArea(child->mAreaCache);
			child->mArea.updateCurrentRect();
		}
	}

	for (auto child : iter->children) {
		updateAreaCache(child, read);
	}
}

void RootWidget::adjustSizes(ActiveTreeNode* iter) {
	if (!iter) return;

	iter->widget->pickRect();

	for (auto child : iter->children) {
		adjustSizes(child);
	}

	iter->widget->adjustChildrenRect();
}

void RootWidget::processActiveTree(ActiveTreeNode* iter, EventHandler& events, Vec2F parent) {
	if (!iter) return;

	auto current = parent + iter->widget->getAreaT().pos;

	if (!iter->widget->mInFocus) {
		iter->widget->mGlobalPoint = current;

		events.setCursorOrigin(current);
		iter->widget->process(events);
	}

	for (auto child : iter->children) {
		processActiveTree(child, events, current);
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
		widgetGlobalPos[widget + 1] = widgetGlobalPos[widget] + path[widget + 1]->getAreaT().pos;
		// path[widget]->mGlobalPoint = widgetGlobalPos[widget];
	}

	bool eventsProcessed = false;

	for (int iter = (int) len - 1; iter >= 0; iter--) {
		auto widget = path[iter];

		events.setCursorOrigin(widgetGlobalPos[iter]);

		widget->mGlobalPoint = widgetGlobalPos[iter];

		if (!eventsProcessed && widget->processesEvents()) {
			events.setEnableKeyEvents(true);
			widget->process(events);

			events.setEnableKeyEvents(false);
			eventsProcessed = true;
		} else {
			widget->process(events);
		}
	}
}
