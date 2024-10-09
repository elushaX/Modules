#include "DockLayoutWidget.hpp"

using namespace tp;

DockLayoutWidget::DockLayoutWidget() {
	mSideWidgets[0].side = LEFT;
	mSideWidgets[1].side = TOP;
	mSideWidgets[2].side = RIGHT;
	mSideWidgets[3].side = BOTTOM;
}

void DockLayoutWidget::process(const EventHandler& events) {
	// calculateHeaderAreas();
	// handlePreview(events);

	handleResizeEvents(events);
	calculateSideAreas();
	calculateResizeHandles();
	updateChildSideWidgets();
}

void DockLayoutWidget::draw(Canvas& canvas) {
	canvas.rect(getRelativeArea(), mBackgroundColor, 0);
}

void DockLayoutWidget::drawOverlay(Canvas& canvas) {
	for (auto& sideWidget : mSideWidgets) {
		if (!isSideVisible(sideWidget.side)) continue;
		auto& handle = sideWidget.resizeHandle;

		if (handle.active) {
			canvas.rect(handle.area.shrink(mPadding / 1.5f), mResizeHandleColorActive, 0);
		} else if (handle.hover) {
			canvas.rect(handle.area.shrink(mPadding / 1.5f), mResizeHandleColorHovered, 0);
		}
	}

	for (auto& sideWidget : mSideWidgets) {
		if (!isSideVisible(sideWidget.side)) continue;
		canvas.rect(sideWidget.headerArea, mResizeHandleColorActive, 0);
	}

	if (mPreview) {
		if (mPreviewSide != NONE) canvas.rect(mPreviewArea.shrink(mPadding * 2), mPreviewColor, mRounding);

		for (auto& sideWidget : mSideWidgets) {
			if (sideWidget.widget) continue;
			canvas.rect(sideWidget.previewHandleArea, mPreviewColor, mRounding);
		}
	}
}

bool DockLayoutWidget::propagateEventsToChildren() const {
	return !mResizing;
}

bool DockLayoutWidget::needsNextFrame() const {
	return Widget::needsNextFrame() || mResizing;
}

void DockLayoutWidget::setCenterWidget(Widget* widget) {
	mCenterWidget = widget;
	addChild(widget);
}

void DockLayoutWidget::dockWidget(Widget* widget, Side side) {
	DEBUG_ASSERT(!sideExists(side))

	addChild(widget);

	auto& sideWidget = mSideWidgets[side];
	sideWidget.widget = widget;

	for (auto& order : mSideWidgets) {
		if (order.order == -1) {
			order.order = side;
			break;
		}
	}

	sideWidget.hidden = false;
}

void DockLayoutWidget::undockWidget(Side side) {
	DEBUG_ASSERT(sideExists(side))

	bool removed = false;
	for (ualni i = 0; i < 3; i++) {
		if (mSideWidgets[i].order == side) {
			removed = true;
		}
		if (removed) {
			swap(mSideWidgets[i].order, mSideWidgets[i + 1].order);
		}
	}
	mSideWidgets[3].order = -1;

	mSideWidgets[side].widget = nullptr;
}

void DockLayoutWidget::toggleWidgetVisibility(Side side) {
	DEBUG_ASSERT(sideExists(side))
	mSideWidgets[side].hidden = !mSideWidgets[side].hidden;
}

void DockLayoutWidget::calculateSideAreas() {
	auto startArea = getRelativeArea();

	for (auto& sideWidget : mSideWidgets) {
		const auto side = sideWidget.order;

		if (side == -1) break;
		if (!isSideVisible(Side(side))) continue;

		bool vertical = side == TOP || side == BOTTOM;
		bool opposite = side == BOTTOM || side == RIGHT;

		auto& sideSize = mSideWidgets[side].absoluteSize;

		auto factor = sideSize / startArea.size[vertical];
		if (opposite) factor = factor * -1 + 1;
		auto& area = mSideWidgets[side].area;

		if (!vertical) {
			const auto first = startArea.splitByFactorHL(factor);
			const auto second = startArea.splitByFactorHR(factor);
			area = side == LEFT ? first : second;
			startArea = side == LEFT ? second : first;
		} else {
			const auto first = startArea.splitByFactorVT(factor);
			const auto second = startArea.splitByFactorVB(factor);
			area = side == TOP ? first : second;
			startArea = side == TOP ? second : first;
		}

		area = area.shrink(mPadding);
	}

	mCenterArea = startArea.shrink(mPadding);
}

void DockLayoutWidget::calculateResizeHandles() {
	RectF rec;
	RectF area = getRelativeArea();

	for (auto& sideWidget : mSideWidgets) {
		auto& sideSize = sideWidget.absoluteSize;
		auto& resizeHandle = sideWidget.resizeHandle;

		if (resizeHandle.end < mSideSizePadding * 2) {
			sideSize = resizeHandle.end / 2.f;
		} else {
			sideSize = clamp(sideSize, resizeHandle.start + mSideSizePadding, resizeHandle.end - mSideSizePadding);
		}
	}

	if (isSideVisible(LEFT)) {
		auto& side = mSideWidgets[LEFT];
		rec = { side.area.p4(), { mPadding * 2, side.area.size.y } };
		side.resizeHandle.area = rec;
		side.resizeHandle.start = 0;
		side.resizeHandle.end = mCenterArea.p3().x;
	}
	if (isSideVisible(RIGHT)) {
		auto& side = mSideWidgets[RIGHT];
		rec = { side.area.p1(), { mPadding * 2, side.area.size.y } };
		rec.x -= mPadding * 2;
		side.resizeHandle.area = rec;
		side.resizeHandle.start = 0;
		side.resizeHandle.end = (area.p3() - mCenterArea.p1()).x;
	}
	if (isSideVisible(TOP)) {
		auto& side = mSideWidgets[TOP];
		rec = { side.area.p2(), { side.area.size.x, mPadding * 2 } };
		side.resizeHandle.area = rec;
		side.resizeHandle.start = 0;
		side.resizeHandle.end = mCenterArea.p2().y;
	}
	if (isSideVisible(BOTTOM)) {
		auto& side = mSideWidgets[BOTTOM];
		rec = { side.area.p1(), { side.area.size.x, mPadding * 2 } };
		rec.y -= mPadding * 2;
		side.resizeHandle.area = rec;
		side.resizeHandle.start = 0;
		side.resizeHandle.end = (area.p3() - mCenterArea.p1()).y;
	}
}

void DockLayoutWidget::handleResizeEvents(const EventHandler& events) {
	auto pointer = events.getPointer();
	auto pointerDelta = events.getPointerDelta();

	// check triggers
	if (events.isReleased(InputID::MOUSE1)) {
		for (auto& sideWidget : mSideWidgets) {
			sideWidget.resizeHandle.active = false;
		}

		mResizing = false;
	} else {
		for (auto& sideWidget : mSideWidgets) {
			sideWidget.resizeHandle.hover = sideWidget.resizeHandle.area.isInside(pointer);
			if (sideWidget.resizeHandle.hover && events.isPressed(InputID::MOUSE1)) {
				sideWidget.resizeHandle.active = true;

				mResizing = true;
			  triggerWidgetUpdate();
			}
		}
	}

	// do the resizing
	for (auto& sideWidget : mSideWidgets) {
		if (!sideWidget.resizeHandle.active) continue;

		halnf delta = pointerDelta[(sideWidget.side == TOP || sideWidget.side == BOTTOM)];

		if (sideWidget.side == BOTTOM || sideWidget.side == RIGHT) {
			delta *= -1;
		}

		sideWidget.absoluteSize += delta;
	}
}

void DockLayoutWidget::updateChildSideWidgets() {
	// Update Child Widgets
	{
		for (ualni i = 0; i < 4; i++) {
			if (!sideExists(Side(i))) continue;
			auto widget = mSideWidgets[i].widget;

			if (!isSideVisible(Side(i))) {
				// widget->mEnable = false;
			} else {
				widget->setArea(mSideWidgets[i].area);
				// widget->mEnable = true;
			}
		}

		if (mCenterWidget) mCenterWidget->setArea(mCenterArea);
	}

	// update depth order
	/*
		if (mChildWidgets.size() > 1) {
			for (auto sideChild : mSideWidgets) {
				if (!sideChild) continue;

		for (auto childNode = mChildWidgets.firstNode(); childNode; childNode = childNode->next) {
			if (childNode->data == sideChild) {
				mChildWidgets.detach(childNode);
				mChildWidgets.pushBack(childNode);
				break;
			}
		}
	}
	}
	*/
}

void DockLayoutWidget::calculateHeaderAreas() {
	for (ualni i = 0; i < 4; i++) {
		if (!isSideVisible(Side(i))) continue;
		auto& area = mSideWidgets[i].area;
		const auto factor = mHeaderSize / area.size.y;
		mSideWidgets[i].headerArea = area.splitByFactorVT(factor);
		area = area.splitByFactorVB(factor);

		mSideWidgets[i].headerArea.size.y -= mPadding;
	}
}

bool DockLayoutWidget::isSideVisible(DockLayoutWidget::Side side) {
	return sideExists(side) && !mSideWidgets[side].hidden;
}

bool DockLayoutWidget::sideExists(DockLayoutWidget::Side side) { return mSideWidgets[side].widget; }

ualni DockLayoutWidget::getVisibleSidesSize() {
	ualni out = 0;
	for (ualni i = 0; i < 4; i++) {
		if (isSideVisible(Side(i))) out++;
	}
	return out;
}

void DockLayoutWidget::handlePreview(const EventHandler& events) {
	if (!mPreview) {
		mPreviewSide = NONE;
		return;
	}

	const halnf factor = 0.3;
	const halnf handleFactor = 0.1;

	const auto handleSize = min(mCenterArea.size.x, mCenterArea.size.y) * handleFactor;

	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.widget) continue;

		switch (sideWidget.side) {
			case TOP: sideWidget.area = mCenterArea.splitByFactorVT(factor); break;
			case BOTTOM: sideWidget.area = mCenterArea.splitByFactorVB(1 - factor); break;
			case LEFT: sideWidget.area = mCenterArea.splitByFactorHL(factor); break;
			case RIGHT: sideWidget.area = mCenterArea.splitByFactorHR(1 - factor); break;
			default: break;
		}

		sideWidget.area = sideWidget.area.shrink(mPadding * 2);
		sideWidget.previewHandleArea = sideWidget.area.getSizedFromCenter({ handleSize, handleSize });
	}

	mPreviewArea = {};
	mPreviewSide = NONE;
	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.widget) continue;
		
		if (sideWidget.previewHandleArea.isInside(events.getPointer())) {
			mPreviewArea = sideWidget.area;
			mPreviewSide = sideWidget.side;
		}
	}
}
