#include "GridLayoutWidget.hpp"

using namespace tp;

GridLayoutWidget::GridLayoutWidget() {
	mSideWidgets[0].side = LEFT;
	mSideWidgets[1].side = TOP;
	mSideWidgets[2].side = RIGHT;
	mSideWidgets[3].side = BOTTOM;
}

void GridLayoutWidget::addSideWidget(Widget* widget, Side side) {
	if (isSideExists(side)) return;

	auto& sideWidget = mSideWidgets[side];
	sideWidget.widget = widget;
	for (auto& order : mSideWidgets) {
		if (order.order == -1) {
			order.order = side;
			break;
		}
	}

	sideWidget.hidden = false;

	mChildWidgets.pushBack(widget);
	widget->mIsDocked = true;
}

void GridLayoutWidget::removeSideWidget(Side side) {
	if (!isSideExists(side)) return;

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

	auto widget = mSideWidgets[side].widget;
	widget->mIsDocked = false;
	mChildWidgets.removeNode(mChildWidgets.find(widget));

	mSideWidgets[side].widget = nullptr;
}

void GridLayoutWidget::setCenterWidget(Widget* widget) {
	mChildWidgets.removeNode(mChildWidgets.find(mCenterWidget));
	mCenterWidget = widget;
	mChildWidgets.pushBack(mCenterWidget);
}

void GridLayoutWidget::toggleHiddenState(GridLayoutWidget::Side side) {
	if (!isSideExists(side)) return;
	mSideWidgets[side].hidden = !mSideWidgets[side].hidden;
}

void GridLayoutWidget::eventProcess(const tp::Events& events) {
	calculateSideAreas();
	calculateResizeHandles();
	// calculateHeaderAreas();

	handlePreview(events);
	handleResizeEvents(events);

	updateChildSideWidgets();
}

void GridLayoutWidget::eventDraw(Canvas& canvas) {
	canvas.rect(this->mArea, mBackgroundColor, 0);

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
}

void GridLayoutWidget::eventDrawOver(Canvas& canvas) {
	if (!mPreview) return;

	if (mPreviewSide != NONE) canvas.rect(mPreviewArea.shrink(mPadding * 2), mPreviewColor, mRounding);

	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.widget) continue;
		canvas.rect(sideWidget.previewHandleArea, mPreviewColor, mRounding);
	}
}

void GridLayoutWidget::calculateSideAreas() {
	auto startArea = this->mArea;
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

void GridLayoutWidget::calculateResizeHandles() {
	RectF rec;

	if (isSideVisible(LEFT)) {
		auto& side = mSideWidgets[LEFT];
		rec = { side.area.p4(), { mPadding * 2, side.area.size.y } };
		side.resizeHandle = { rec, 0, mCenterArea.p3().x };
	}
	if (isSideVisible(RIGHT)) {
		auto& side = mSideWidgets[RIGHT];
		rec = { side.area.p1(), { mPadding * 2, side.area.size.y } };
		rec.x -= mPadding * 2;
		side.resizeHandle = { rec, 0, (this->mArea.p3() - mCenterArea.p1()).x };
	}
	if (isSideVisible(TOP)) {
		auto& side = mSideWidgets[TOP];
		rec = { side.area.p2(), { side.area.size.x, mPadding * 2 } };
		side.resizeHandle = { rec, 0, mCenterArea.p2().y };
	}
	if (isSideVisible(BOTTOM)) {
		auto& side = mSideWidgets[BOTTOM];
		rec = { side.area.p1(), { side.area.size.x, mPadding * 2 } };
		rec.y -= mPadding * 2;
		side.resizeHandle = { rec, 0, (this->mArea.p3() - mCenterArea.p1()).y };
	}
}

void GridLayoutWidget::handleResizeEvents(const Events& events) {
	for (auto& sideWidget : mSideWidgets) {
		auto& sideSize = sideWidget.absoluteSize;
		auto& resizeHandle = sideWidget.resizeHandle;
		if (resizeHandle.end < mSideSizePadding * 2) {
			sideSize = resizeHandle.end / 2.f;
		} else {
			sideSize = clamp(sideSize, resizeHandle.start + mSideSizePadding, resizeHandle.end - mSideSizePadding);
		}
	}

	for (auto& sideWidget : mSideWidgets) {
		sideWidget.resizeHandle.hover = false;
		if (sideWidget.resizeHandle.area.isInside(events.getPointerPrev())) {
			sideWidget.resizeHandle.hover = true;
		}
	}

	for (auto& sideWidget : mSideWidgets) {
		sideWidget.resizeHandle.active = false;
	}

	if (mPreview) return;

	auto resizeSideWidget = [&events](SideWidgetData& sideWidget, Vec2F deltaVec) {
		halnf delta = deltaVec[(sideWidget.side == TOP || sideWidget.side == BOTTOM)];
		if (sideWidget.side == BOTTOM || sideWidget.side == RIGHT) delta *= -1;
		sideWidget.absoluteSize += delta;
		sideWidget.resizeHandle.active = true;
	};

	if (events.isDown(InputID::MOUSE1)) {
		for (auto& sideWidget : mSideWidgets) {
			if (sideWidget.resizeHandle.area.isInside(events.getPointerPrev())) {
				resizeSideWidget(sideWidget, events.getPointerDelta());
			}
		}
	}

	if (events.isDown(InputID::LEFT_ALT)) {
		const auto pointer = events.getPointerPrev();
		if (!mCenterArea.isInside(pointer)) return;

		if (events.isPressed(InputID::MOUSE1)) {
			for (auto i : { 0, 1 }) {
				const auto step = mCenterArea.size[i] / 3.f;
				const auto rec1 = mCenterArea.pos[i];
				const auto pos = pointer[i];
				if (pos > rec1 && pos < rec1 + step) resizeType[i] = 0;
				if (pos > rec1 + step && pos < rec1 + step * 2) resizeType[i] = 1;
				if (pos > rec1 + step * 2) resizeType[i] = 2;
			}
		} else if (events.isDown(InputID::MOUSE1)) {
			const auto deltaVec = events.getPointerDelta();

			if (resizeType[0] == 0) resizeSideWidget(mSideWidgets[LEFT], deltaVec);
			if (resizeType[1] == 0) resizeSideWidget(mSideWidgets[TOP], deltaVec);
			if (resizeType[0] == 2) resizeSideWidget(mSideWidgets[RIGHT], deltaVec);
			if (resizeType[1] == 2) resizeSideWidget(mSideWidgets[BOTTOM], deltaVec);
		}
	}
}

void GridLayoutWidget::updateChildSideWidgets() {
	// Update Child Widgets
	{
		for (ualni i = 0; i < 4; i++) {
			if (!isSideExists(Side(i))) continue;
			auto widget = mSideWidgets[i].widget;

			if (!isSideVisible(Side(i))) {
				widget->mEnable = false;
			} else {
				widget->setArea(mSideWidgets[i].area);
				widget->mEnable = true;
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

void GridLayoutWidget::calculateHeaderAreas() {
	for (ualni i = 0; i < 4; i++) {
		if (!isSideVisible(Side(i))) continue;
		auto& area = mSideWidgets[i].area;
		const auto factor = mHeaderSize / area.size.y;
		mSideWidgets[i].headerArea = area.splitByFactorVT(factor);
		area = area.splitByFactorVB(factor);

		mSideWidgets[i].headerArea.size.y -= mPadding;
	}
}

bool GridLayoutWidget::isSideVisible(GridLayoutWidget::Side side) {
	return isSideExists(side) && !mSideWidgets[side].hidden;
}

bool GridLayoutWidget::isSideExists(GridLayoutWidget::Side side) { return mSideWidgets[side].widget; }

ualni GridLayoutWidget::getVisibleSidesSize() {
	ualni out = 0;
	for (ualni i = 0; i < 4; i++) {
		if (isSideVisible(Side(i))) out++;
	}
	return out;
}

GridLayoutWidget::Side GridLayoutWidget::getPreviewSide() { return mPreviewSide; }

void GridLayoutWidget::handlePreview(const Events& events) {
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