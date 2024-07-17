#include "RootWidget.hpp"

using namespace tp;

void RootWidget::setRootWidget(Widget* widget) { mRoot = widget; }

void RootWidget::processFrame(EventHandler* events, const RectF& screenArea) {
	mNeedUpdate = false;

	auto currentPaths = &mActivePaths[mFlipFlop];
	auto prevPaths = &mActivePaths[!mFlipFlop];

	updateActivePaths(currentPaths, events->getPointer());

	mRoot->setArea(screenArea);

	clearProcessedFlag(*currentPaths);
	clearProcessedFlag(*prevPaths);

	processPaths(*currentPaths, *events);
	processPaths(*prevPaths, *events);

	mActiveWidgets.erase_if([](auto widget) { return !widget->mIsActive; });

	mFlipFlop = !mFlipFlop;
}

bool RootWidget::needsUpdate() const { return mNeedUpdate; }

void RootWidget::drawFrame(Canvas& canvas) {
	// draw from top to bottom
	drawRecursion(canvas, mRoot, { 0, 0 });
}

void RootWidget::drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos) {
	canvas.setOrigin(pos);
	canvas.pushClamp({ pos, active->getArea().size });

	active->draw(canvas);
	for (auto child : active->mChildWidgets) {
		drawRecursion(canvas, child.data(), pos + child->getArea().pos);
	}

	canvas.setOrigin(pos);
	canvas.popClamp();

	active->drawOverlay(canvas);
}

void RootWidget::updateActivePaths(Buffer<ActivePath>* activePaths, const Vec2F& pointer) {
	activePaths->clear();

	// based on pointer position
	Widget* pointedWidget = nullptr;

	{
		ActivePath& activePath = activePaths->append(ActivePath{});
		activePath.path.append({ mRoot, {} });

		auto pointerIter = pointer;

		bool newActive = true;
		while (newActive) {
			newActive = false;
			for (auto child : activePath.path.last().widget->mChildWidgets) {
				const auto area = child->mArea.getCurrentRect();
				if (area.isInside(pointerIter)) {
					activePath.path.append({ child.data(), {} });
					newActive = true;
					pointerIter -= child->getArea().pos;
					break;
				}
			}
		}

		pointedWidget = activePath.path.last().widget;
	}

	// based on individual widget requirements
	{
		for (auto widget : mActiveWidgets) {
			if (widget.data() == pointedWidget) continue;

			ActivePath& activePath = activePaths->append(ActivePath{});

			for (Widget* iter = widget.data(); iter; iter = iter->mParent) {
				activePath.path.append({ iter, {} });
			}

			activePath.path.reverse();
		}
	}

	// update paths global pos
	for (auto activePath : *activePaths) {
		Vec2F iterPos = { 0, 0 };
		for (auto activeWidget : activePath->path) {
			iterPos += activeWidget->widget->getArea().pos;
			activeWidget->globalPos = iterPos;
		}
	}
}

void RootWidget::setWidgetArea(Widget& widget, const RectF& rect) {
	widget.mArea.setTargetRect(rect);
	widget.mArea.endAnimation();
}

void RootWidget::processPaths(const Buffer<ActivePath>& paths, EventHandler& events) {
	EventHandler dummyEvents;
	bool eventsHandled = false;

	for (auto path : paths) {

		for (alni widgetIdx = (alni) path->path.size() - 1; widgetIdx >= 0; widgetIdx--) {
			auto activeWidgetNode = path->path[widgetIdx];
			auto activeWidget = activeWidgetNode.widget;

			activeWidget->mArea.updateCurrentRect();

			events.setCursorOrigin(activeWidgetNode.globalPos);

			bool useEvents = !eventsHandled || activeWidget->mIsActive;

			if (!activeWidget->mProcessedFlag) {
				activeWidget->process(useEvents ? events : dummyEvents);
				activeWidget->mProcessedFlag = true;
			}

			eventsHandled = !activeWidget->isPassThroughEvents();

			auto area = activeWidget->mArea.getCurrentRect();
			activeWidget->updateArea(area);
			activeWidget->setArea(area);

			mNeedUpdate |= activeWidget->needUpdate();

			if (!activeWidget->needUpdate()) activeWidget->finishAnimations();

			if (activeWidget->mIsActive) {
				if (mActiveWidgets.find(activeWidget) == -1) {
					mActiveWidgets.append(activeWidget);
				}
			}
		}
	}
}

void RootWidget::clearProcessedFlag(const Buffer<ActivePath>& paths) {
	for (auto path : paths) {
		for (auto widget : path->path) {
			widget->widget->mProcessedFlag = false;
		}
	}
}
