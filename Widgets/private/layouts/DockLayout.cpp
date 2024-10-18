#include "DockLayout.hpp"

using namespace tp;

DockLayout::DockLayout(Widget* widget) :
	WidgetLayout(widget) {
	mSideWidgets[0].side = LEFT;
	mSideWidgets[1].side = TOP;
	mSideWidgets[2].side = RIGHT;
	mSideWidgets[3].side = BOTTOM;
}

bool DockLayout::setCenterWidget(Widget* widget) {
	if (mCenterWidget) return false;
	mCenterWidget = widget;
	return true;
}

bool DockLayout::dockWidget(Widget* widget, Side side) {
	if (sideExists(side) || side == Side::NONE) return false;

	auto& sideWidget = mSideWidgets[side];
	sideWidget.widget = widget;

	for (auto& order : mSideWidgets) {
		if (order.order == -1) {
			order.order = side;
			break;
		}
	}

	sideWidget.hidden = false;
	sideWidget.areaBeforeDocking = widget->getArea();

	return true;
}

bool DockLayout::undockWidget(Side side) {
	if (!sideExists(side) || (side == Side::NONE)) return false;

	bool removed = false;
	for (ualni i = 0; i < 3; i++) {
		if (mSideWidgets[i].order == side) {
			removed = true;
		}
		if (removed) {
			swapV(mSideWidgets[i].order, mSideWidgets[i + 1].order);
		}
	}
	mSideWidgets[3].order = -1;
	mSideWidgets[side].widget = nullptr;

	return true;
}

void DockLayout::toggleWidgetVisibility(Side side) {
	DEBUG_ASSERT(sideExists(side))
	mSideWidgets[side].hidden = !mSideWidgets[side].hidden;
}

void DockLayout::calculateSideAreas() {
	auto startArea = getArea().relative();

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

void DockLayout::calculateResizeHandles() {
	RectF rec;
	RectF area = getArea().relative();

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

void DockLayout::updateChildSideWidgets() {
	for (ualni i = 0; i < 4; i++) {
		if (!sideExists(Side(i))) continue;
		auto widget = mSideWidgets[i].widget;

		if (!isSideVisible(Side(i))) {
			// FIXME : widget->mEnable = false;
		} else {
			widget->setAreaCache(mSideWidgets[i].area);
			// FIXME : widget->mEnable = true;
		}
	}

	if (mCenterWidget) mCenterWidget->setAreaCache(mCenterArea);
}

void DockLayout::calculateHeaderAreas() {
	for (ualni i = 0; i < 4; i++) {
		if (!isSideVisible(Side(i))) continue;
		auto& area = mSideWidgets[i].area;
		const auto factor = mHeaderSize / area.size.y;
		mSideWidgets[i].headerArea = area.splitByFactorVT(factor);
		area = area.splitByFactorVB(factor);

		mSideWidgets[i].headerArea.size.y -= mPadding;
	}
}

ualni DockLayout::getVisibleSidesSize() const {
	ualni out = 0;
	for (ualni i = 0; i < 4; i++) {
		if (isSideVisible(Side(i))) out++;
	}
	return out;
}

auto DockLayout::getSideFromWidget(Widget* widget) -> Side {
	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.widget == widget) return sideWidget.side;
	}
	return DockLayout::NONE;
}

void DockLayout::updateLayout(bool vertical) {
	for (auto child : children()) {
		child->getLayout()->pickRect(vertical);
	}

	if (vertical) return;
	adjustChildrenRect();
}

void DockLayout::adjustChildrenRect() {
	calculateSideAreas();
	calculateResizeHandles();
	updateChildSideWidgets();
}

void DockLayout::updatePreviewSide(const Vec2F& pointer) {
	const auto handleSize = min(mCenterArea.size.x, mCenterArea.size.y) * mHandleFactor;

	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.widget) continue;

		switch (sideWidget.side) {
			case TOP: sideWidget.area = mCenterArea.splitByFactorVT(mHandleSplitFactor); break;
			case BOTTOM: sideWidget.area = mCenterArea.splitByFactorVB(1 - mHandleSplitFactor); break;
			case LEFT: sideWidget.area = mCenterArea.splitByFactorHL(mHandleSplitFactor); break;
			case RIGHT: sideWidget.area = mCenterArea.splitByFactorHR(1 - mHandleSplitFactor); break;
			default: break;
		}

		sideWidget.area = sideWidget.area.shrink(mPadding * 2);
		sideWidget.previewHandleArea = sideWidget.area.getSizedFromCenter({ handleSize, handleSize });
	}

	mPreviewArea = {};
	mPreviewSide = NONE;

	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.widget) continue;

		if (sideWidget.previewHandleArea.isInside(pointer)) {
			mPreviewArea = sideWidget.area;
			mPreviewSide = sideWidget.side;
		}
	}
}

DockLayout::Side DockLayout::getPreviewSide() {
	return mPreviewSide;
}

void DockLayout::updateResizeHover(const Vec2F& pointer) {
	for (auto& sideWidget : mSideWidgets) {
		sideWidget.resizeHandle.hover = sideWidget.resizeHandle.area.isInside(pointer);
	}
}

void DockLayout::updateResize(const Vec2F& pointerDelta) {
	if (!mResizing) return;

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

bool DockLayout::startResize(const Vec2F& pointer) {
	for (auto& sideWidget : mSideWidgets) {
		if (sideWidget.resizeHandle.hover) {
			sideWidget.resizeHandle.active = true;
			mResizing = true;
			return true;
		}
	}
	return false;
}

void DockLayout::endResize() {
	for (auto& sideWidget : mSideWidgets) {
		sideWidget.resizeHandle.active = false;
	}
	mResizing = false;
}

bool DockLayout::isSideVisible(DockLayout::Side side) const {
	return sideExists(side) && !mSideWidgets[side].hidden;
}

bool DockLayout::sideExists(DockLayout::Side side) const {
	return mSideWidgets[side].widget;
}

const std::vector<Widget*>& DockLayout::getDockedWidgets() {
  mDockedWidgets.resize(5);
  for (auto i = 0; i < 4; i++) {
    mDockedWidgets[i] = mSideWidgets[i].widget;
  }
  mDockedWidgets[4] = mCenterWidget;
  return mDockedWidgets;
}