#include "DockWidget.hpp"
#include "FloatingWidget.hpp"

using namespace tp;

DockWidget::DockWidget() : Widget() {
	setDebug("dock", { 1, 1, 1, 1 });
	setLayout(new DockLayout(this));
}

void DockWidget::dockWidget(Widget* widget, DockLayout::Side side) {
	if (layout()->dockWidget(widget, side)) {
		addChild(widget);

		for (auto child : layout()->getDockedWidgets()) {
      if (child) child->bringToBack();
		}
	}
}

void DockWidget::undockWidget(DockLayout::Side side, bool restoreArea) {
	if (layout()->sideExists(side) && side != DockLayout::NONE) {
		auto widget = layout()->getSideWidget(side);

		widget->bringToFront();
		if (restoreArea) {
			widget->setArea(layout()->getRectBeforeDocked(side));
		} else {
			// FIXME : widget->endAnimations();
		}

		layout()->undockWidget(side);
	}
}

void DockWidget::setCenterWidget(Widget* widget) {
	if (layout()->setCenterWidget(widget)) {
		addChild(widget);
	}
}

void DockWidget::toggleWidgetVisibility(DockLayout::Side side) {
	if (layout()->sideExists(side)) {
		auto widget = layout()->getSideWidget(side);
		widget->setEnabled(!layout()->isSideVisible(side));
		widget->bringToBack();

		layout()->toggleWidgetVisibility(side);
	}
}

void DockWidget::process(const EventHandler& events) {
	// calculateHeaderAreas();

	Widget* floater = nullptr;
	for (auto child : mChildren) {
		if (auto iter = dynamic_cast<FloatingWidget*>(child)) {
			if (iter->isFloating()) {
				floater = iter;
				break;
			}
		}
	}

	if (floater) undockWidget(layout()->getSideFromWidget(floater), false);

	if (floater) {
		layout()->updatePreviewSide(events.getPointer());
	}

	if (mPreviewWidget && !floater) {
		dockWidget(mPreviewWidget, layout()->getPreviewSide());
	}

	mPreviewWidget = floater;

	if (!mPreviewWidget) {
		layout()->updateResizeHover(events.getPointer());

		if (events.isPressed(InputID::MOUSE1)) {
			if (layout()->startResize(events.getPointer())) {
				triggerWidgetUpdate("dock layout resizing");
			}
		} else if (events.isReleased(InputID::MOUSE1)) {
			layout()->endResize();
		}

		if (layout()->isResizing()) {
			layout()->updateResize(events.getPointerDelta());
		}
	}
}

bool DockWidget::propagateEventsToChildren() const {
	return !layout()->isResizing();
}

bool DockWidget::needsNextFrame() const {
	return Widget::needsNextFrame() || layout()->isResizing();
}

void DockWidget::draw(Canvas& canvas) {
	canvas.rect(getRelativeArea(), mBackgroundColor, 0);
}

void DockWidget::drawSide(DockLayout::Side side, tp::Canvas& canvas) {
	auto lay = layout();

	if (lay->isSideVisible(side)) {
		// header
		canvas.rect(lay->getHeaderArea(side), mResizeHandleColorActive, 0);

		// resize
		if (lay->isResizing(side)) {
			canvas.rect(lay->getResizeHandleArea(side).shrink(mPadding / 1.5f), mResizeHandleColorActive, 0);
		} else if (lay->isResizeHandleHover(side)) {
			canvas.rect(lay->getResizeHandleArea(side).shrink(mPadding / 1.5f), mResizeHandleColorHovered, 0);
		}

	} else {
		// preview handles
		if (mPreviewWidget) {
			// preview active
			if (lay->getPreviewSide() == side) {
				canvas.rect(lay->getPreviewArea().shrink(mPadding * 2), mPreviewColor, mRounding);
			} else {
				canvas.rect(lay->getPreviewHandleArea(side), mPreviewColor, mRounding);
			}
		}
	}
}

void DockWidget::drawOverlay(Canvas& canvas) {
	drawSide(DockLayout::Side::LEFT, canvas);
	drawSide(DockLayout::Side::RIGHT, canvas);
	drawSide(DockLayout::Side::TOP, canvas);
	drawSide(DockLayout::Side::BOTTOM, canvas);
}