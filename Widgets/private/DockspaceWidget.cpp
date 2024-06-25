#include "DockspaceWidget.hpp"

using namespace tp;

DockSpaceWidget::DockSpaceWidget() = default;

void DockSpaceWidget::addSideWidget(Widget* widget, Side side) {
	if (mSideWidgets[side]) return;
	mSideWidgets[side] = widget;
	for (auto& order : mSideSplitOrder) {
		if (order == -1) {
			order = side;
			break;
		}
	}
	mChildWidgets.pushBack(widget);
	widget->mIsDocked = true;
	mSideHidden[side] = false;
}

void DockSpaceWidget::removeSideWidget(Side side) {
	if (!mSideWidgets[side]) return;

	bool removed = false;
	for (ualni i = 0; i < 3; i++) {
		if (mSideSplitOrder[i] == side) {
			removed = true;
		}
		if (removed) {
			mSideSplitOrder[i] = mSideSplitOrder[i + 1];
		}
	}
	mSideSplitOrder[3] = -1;

	auto widget = mSideWidgets[side];
	mSideWidgets[side] = nullptr;
	widget->mIsDocked = false;

	mChildWidgets.removeNode(mChildWidgets.find(widget));
}

void DockSpaceWidget::setCenterWidget(Widget* widget) {
	mChildWidgets.removeNode(mChildWidgets.find(mCenterWidget));
	mCenterWidget = widget;
	mChildWidgets.pushBack(mCenterWidget);
}

void DockSpaceWidget::toggleHiddenState(DockSpaceWidget::Side side) {
	if (!isSideExists(side)) return;
	mSideHidden[side] = !mSideHidden[side];
}

void DockSpaceWidget::eventProcess(const tp::Events& events) {
	updateActiveWindow(events);

	calculateSideAreas();
	calculateResizeHandles();
	// calculateHeaderAreas();
	handleResizeEvents(events);

	updateChildSideWidgets();
}

void DockSpaceWidget::eventDraw(Canvas& canvas) {
	canvas.rect(this->mArea, mBackgroundColor, 0);

	for (auto sizeHandle : mResizeHandles) {
		if (sizeHandle->active) {
			canvas.rect(sizeHandle->area.shrink(mPadding / 1.5f), mResizeHandleColorActive, 0);
		} else if (sizeHandle->hover) {
			canvas.rect(sizeHandle->area.shrink(mPadding / 1.5f), mResizeHandleColorHovered, 0);
		}
	}

	for (auto& header : mSideHeaderAreas) {
		canvas.rect(header, mResizeHandleColorActive, 0);
	}
}

void DockSpaceWidget::updateActiveWindow(const tp::Events& events) {
	if (this->mChildWidgets.size() > 1 && events.isPressed(InputID::MOUSE1)) {

		Widget* activeChild = nullptr;

		for (auto childNode = this->mChildWidgets.firstNode(); childNode; childNode = childNode->next) {
			auto child = childNode->data;

			if (child->mIsDocked) continue;
			if (child->mArea.isInside(events.getPointer())) {

				mChildWidgets.detach(childNode);
				mChildWidgets.pushFront(childNode);

				child->mHandlesEvents = true;
				activeChild = child;
				break;
			}
		}

		if (activeChild) {
			for (auto child : this->mChildWidgets) {
				if (activeChild != child.data() && !child->mIsDocked) child->mHandlesEvents = false;
			}
		}
	}
}

void DockSpaceWidget::calculateSideAreas() {
	auto startArea = this->mArea;
	for (ualni i = 0; i < 4; i++) {
		const auto side = mSideSplitOrder[i];

		if (side == -1) break;
		if (mSideHidden[side]) continue;

		bool vertical = side == TOP || side == BOTTOM;
		bool opposite = side == BOTTOM || side == RIGHT;

		const auto maxAbsoluteSize = startArea.size[vertical];
		const auto paddingFactor = mSideSizePadding / maxAbsoluteSize;

		mSideAbsoluteSize[side] =
			clamp(mSideAbsoluteSize[side], maxAbsoluteSize * paddingFactor, maxAbsoluteSize * (1.f - paddingFactor));

		auto factor = mSideAbsoluteSize[side] / startArea.size[vertical];
		if (opposite) factor = factor * -1 + 1;

		if (!vertical) {
			const auto first = startArea.splitByFactorHL(factor);
			const auto second = startArea.splitByFactorHR(factor);
			mSideAreas[side] = side == LEFT ? first : second;
			startArea = side == LEFT ? second : first;
		} else {
			const auto first = startArea.splitByFactorVT(factor);
			const auto second = startArea.splitByFactorVB(factor);
			mSideAreas[side] = side == TOP ? first : second;
			startArea = side == TOP ? second : first;
		}

		mSideAreas[side] = mSideAreas[side].shrink(mPadding);
	}

	mCenterArea = startArea.shrink(mPadding);
}

void DockSpaceWidget::calculateResizeHandles() {
	mResizeHandles.clear();
	RectF rec;

	if (isSideVisible(LEFT)) {
		rec = { mSideAreas[LEFT].p4(), { mPadding * 2, mSideAreas[LEFT].size.y } };
		mResizeHandles.append({ rec, LEFT, 0, mCenterArea.p3().x });
	}
	if (isSideVisible(RIGHT)) {
		rec = { mSideAreas[RIGHT].p1(), { mPadding * 2, mSideAreas[RIGHT].size.y } };
		rec.x -= mPadding * 2;
		mResizeHandles.append({ rec, RIGHT, mCenterArea.p1().x, rec.p3().x });
	}
	if (isSideVisible(TOP)) {
		rec = { mSideAreas[TOP].p2(), { mSideAreas[TOP].size.x, mPadding * 2 } };
		mResizeHandles.append({ rec, TOP, 0, mCenterArea.p2().y });
	}
	if (isSideVisible(BOTTOM)) {
		rec = { mSideAreas[BOTTOM].p1(), { mSideAreas[BOTTOM].size.x, mPadding * 2 } };
		rec.y -= mPadding * 2;
		mResizeHandles.append({ rec, BOTTOM, mCenterArea.p1().y, rec.p2().y });
	}
}

void DockSpaceWidget::handleResizeEvents(const Events& events) {
	for (auto resizeHandle : mResizeHandles) {
		resizeHandle->hover = false;
		if (resizeHandle->area.isInside(events.getPointerPrev())) {
			resizeHandle->hover = true;
		}
	}

	if (events.isDown(InputID::MOUSE1)) {
		for (auto resizeHandle : mResizeHandles) {
			resizeHandle->active = false;
			if (resizeHandle->area.isInside(events.getPointerPrev())) {
				auto factorDeltaVec = events.getPointerDelta();
				halnf factorDelta = factorDeltaVec[(resizeHandle->side == TOP || resizeHandle->side == BOTTOM)];
				if (resizeHandle->side == BOTTOM || resizeHandle->side == RIGHT) factorDelta *= -1;
				mSideAbsoluteSize[resizeHandle->side] += factorDelta;
				resizeHandle->active = true;
			}
		}
	}
}

void DockSpaceWidget::updateChildSideWidgets() {
	// Update Child Widgets
	{
		for (ualni i = 0; i < 4; i++) {
			if (!isSideExists(Side(i))) continue;

			if (!isSideVisible(Side(i))) {
				mSideWidgets[i]->mEnable = false;
			} else {
				mSideWidgets[i]->setArea(mSideAreas[i]);
				mSideWidgets[i]->mEnable = true;
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

void DockSpaceWidget::calculateHeaderAreas() {
	for (ualni i = 0; i < 4; i++) {
		if (!isSideVisible(Side(i))) continue;
		const auto factor = mHeaderSize / mSideAreas[i].size.y;
		mSideHeaderAreas[i] = mSideAreas[i].splitByFactorVT(factor);
		mSideAreas[i] = mSideAreas[i].splitByFactorVB(factor);

		mSideHeaderAreas[i].size.y -= mPadding;
	}
}

bool DockSpaceWidget::isSideVisible(DockSpaceWidget::Side side) { return mSideWidgets[side] && !mSideHidden[side]; }

bool DockSpaceWidget::isSideExists(DockSpaceWidget::Side side) { return mSideWidgets[side]; }


ualni DockSpaceWidget::getVisibleSidesSize() {
	ualni out = 0;
	for (ualni i = 0; i < 4; i++) {
		if (isSideVisible(Side(i))) out++;
	}
	return out;
}
