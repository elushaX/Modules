#include "UpdateManager.hpp"
#include "DebugManager.hpp"

using namespace tp;

void UpdateManager::processWidgets(Widget* root, EventHandler& events) {
	events.setEnableKeyEvents(true);
	events.setCursorOrigin({ 0, 0 });

	processFocusItems(events);

	events.setEnableKeyEvents(false);
	events.setCursorOrigin({ 0, 0 });

	processActiveTree(root, events, { 0, 0 });
}

void UpdateManager::clean() {
	erase_if(mTriggeredWidgets, [](auto iter) {
		auto widget = iter.first;
		auto flag = iter.second;

		if (!flag) return false;

		// if (mWidgetsToProcess.find(widget) == mWidgetsToProcess.end()) return false;
		widget->updateAnimations();
		auto end = !widget->needsNextFrame();
		if (end) {
			widget->endAnimations();
			widget->mDebug.triggerReason = "del";
		}
		return end;
	});
}

void UpdateManager::scheduleUpdate(Widget* widget, const char* reason) {
	widget->mDebug.triggerReason = reason;
	mTriggeredWidgets.insert({ widget, false });
}

void UpdateManager::updateTreeToProcess(Widget* root) {
	Widget::dfs(root, [](Widget* widget) {
		widget->mFlags.set(Widget::NEEDS_UPDATE, false);
	});

	for (auto& [widget, flag] : mTriggeredWidgets) {
		flag = true;

		for (auto iter = widget; iter && iter->mParent; iter = iter->mParent) {
			iter->mFlags.set(Widget::NEEDS_UPDATE, true);
		}
	}

	mDebugWidgetsToProcess = 0;
	Widget::dfs(root, [this](Widget*) {
		mDebugWidgetsToProcess++;
	});
}

void UpdateManager::getWidgetPath(Widget* widget, std::vector<Widget*>& out) {
	if (!widget) return;

	for (auto iter = widget; iter && iter->mParent; iter = iter->mParent) {
		out.push_back(iter);
	}

	for (auto i = 0; i < out.size() / 2; i++) {
		swapV(out[i], out[out.size() - i - 1]);
	}
}

Widget* UpdateManager::findFocusWidget(Widget* root, EventHandler& events) {
	events.setCursorOrigin({ 0, 0 });

	mInFocusWidget = nullptr;
	findMouseFocusWidget(root, &mInFocusWidget, events.getPointer());

	if (mFocusLockStack.size()) {
		bool hasLockedWidget = false;
		for (auto iter = mInFocusWidget; iter; iter = iter->mParent) {
			if (iter == mFocusLockStack.last()) {
				hasLockedWidget = true;
				break;
			}
		}
		if (!hasLockedWidget) {
			mInFocusWidget = mFocusLockStack.last();
		}
	}

	return mInFocusWidget;
}

void UpdateManager::handleFocusChanges(Widget* active, Widget* prevActive) {
	// if (mInFocusWidget == prevFocus) return;
	if (active) scheduleUpdate(active, "focus entered");

	if (!active && !prevActive) return;

	std::vector<Widget*> path2;
	getWidgetPath(active, path2);
	size_t propLen2 = path2.size();
	for (auto i = 0; i < path2.size(); i++) {
		if (!path2[i]->propagateEventsToChildren()) {
			propLen2 = i + 1;
			break;
		}
	}

	std::vector<Widget*> path1;
	getWidgetPath(prevActive, path1);
	size_t propLen1 = path1.size();
	for (auto i = 0; i < path1.size(); i++) {
		if (!path1[i]->propagateEventsToChildren()) {
			propLen1 = i + 1;
			break;
		}
	}

	int mostCommonIdx = 0;
	if (!(path1.empty() || path2.empty())) {
		while (mostCommonIdx < min(path1.size(), path2.size()) && path1[mostCommonIdx] == path2[mostCommonIdx]) {
			mostCommonIdx++;
		}
	}
	mostCommonIdx--;

	for (auto i = 0; i < path1.size(); i++) {
		path1[i]->mFlags.set(Widget::IN_FOCUS, false);
		if (i > mostCommonIdx && i < propLen1) path1[i]->mouseLeave();
	}

	for (auto i = 0; i < path2.size(); i++) {
		path2[i]->mFlags.set(Widget::IN_FOCUS, true);
		if (i > mostCommonIdx && i < propLen2) path2[i]->mouseEnter();
	}
}

void UpdateManager::findMouseFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer) {
	if (!iter->mArea.getTargetRect().isInside(pointer) || !iter->mFlags.get(Widget::ENABLED)) return;

	if (iter->processesEvents()) {
		*focus = iter;
	}

	for (auto child = iter->mDepthOrder.lastNode(); child; child = child->prev) {
		findMouseFocusWidget(child->data, focus, pointer - iter->mArea.getTargetRect().pos);
	}
}

void UpdateManager::processActiveTree(Widget* iter, EventHandler& events, Vec2F parent) {
	if (!iter || !iter->isUpdate()) return;

	auto current = parent + iter->getAreaT().pos;

	if (!iter->mFlags.get(Widget::IN_FOCUS)) {
		iter->mDebug.pGlobal = current;

		events.setCursorOrigin(current);
		procWidget(iter, events, false);
	}

	for (auto child : iter->mDepthOrder) {
		processActiveTree(child.data(), events, current);
	}
}

void UpdateManager::processFocusItems(EventHandler& events) {
	if (!mInFocusWidget) return;

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

		widget->mDebug.pGlobal = widgetGlobalPos[iter];

		if (!eventsProcessed && widget->processesEvents()) {
			procWidget(widget, events, true);
			eventsProcessed = true;
		} else {
			procWidget(widget, events, false);
		}
	}
}

void UpdateManager::procWidget(Widget* widget, EventHandler& events, bool withEvents) {
	events.setEnableKeyEvents(withEvents);
	gDebugWidget.checkProcBreakPoints(widget);
	widget->process(events);
}

void UpdateManager::lockFocus(tp::Widget* widget) {
	DEBUG_ASSERT(mFocusLockStack.find(widget) == nullptr)
	// TODO : check that widget is a child of top of the stack
	mFocusLockStack.pushBack(widget);
}

void UpdateManager::freeFocus(tp::Widget* widget) {
	DEBUG_ASSERT(mFocusLockStack.last() == widget)
	mFocusLockStack.popBack();
}

bool UpdateManager::canRemoveWidget(Widget* widget) const {
	if (auto node = mFocusLockStack.find(widget)) {
		if (node->next) return false;
	}

	return true;
}

void UpdateManager::removeWidget(Widget* widget) {
	freeFocus(widget);
}
