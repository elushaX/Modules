#include "WorkspaceWidget.hpp"

using namespace tp;

WorkspaceWidget::WorkspaceWidget() {
	this->mChildWidgets.pushBack(&mFloatingLayer);
	this->mChildWidgets.pushBack(&mDockSpace);
}

void WorkspaceWidget::eventProcess(const Events& events) {
	mDockSpace.setArea(this->mArea);
	mFloatingLayer.setArea(this->mArea);

	mDockSpace.mPreview = false;
	mDockSpace.mHandlesEvents = !mFloatingLayer.handlesEvent();

	for (auto floatingChild : mFloatingLayer.mChildWidgets) {
		auto widget = dynamic_cast<FloatingWidget*>(floatingChild.data());
		if (!widget) continue;

		if (widget->mDropped) {
			auto side = mDockSpace.getPreviewSide();
			if (side != DockWidget::NONE) {
				mFloatingLayer.mChildWidgets.removeNode(mFloatingLayer.mChildWidgets.find(widget));
				widget->setCollapsed(false);
				widget->stopFloating();
				mDockSpace.addSideWidget(widget, side);
			}
		}

		if (widget->isFloating()) {
			mDockSpace.mHandlesEvents = true;
			mDockSpace.mPreview = true;
		}
	}

	for (auto& dockedChild : mDockSpace.mSideWidgets) {
		auto widget = dynamic_cast<FloatingWidget*>(dockedChild.widget);
		if (!widget) continue;

		if (widget->isFloating()) {
			mDockSpace.removeSideWidget(dockedChild.side);
			mFloatingLayer.mChildWidgets.pushBack(widget);
			widget->mArea.pos = events.getPointer() - 20;
			widget->mArea.size = mDefaultFloatSize;

			widget->mResizable = true;
			// widget->mBorders = true;
		}
	}

	// toggle hide state
	if (events.isDown(InputID::LEFT_ALT)) {
		if (events.isPressed(InputID::N1)) {
			mDockSpace.toggleHiddenState(DockWidget::LEFT);
		} else if (events.isPressed(InputID::N2)) {
			mDockSpace.toggleHiddenState(DockWidget::TOP);
		} else if (events.isPressed(InputID::N3)) {
			mDockSpace.toggleHiddenState(DockWidget::RIGHT);
		} else if (events.isPressed(InputID::N4)) {
			mDockSpace.toggleHiddenState(DockWidget::BOTTOM);
		}
	}

	for (auto& dockedChild : mDockSpace.mSideWidgets) {
		auto widget = dynamic_cast<FloatingWidget*>(dockedChild.widget);
		if (!widget) continue;

		widget->setCollapsed(false);
		widget->mResizable = false;
		// widget->mBorders = false;
	}
}