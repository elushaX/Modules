#include "StackLayout.hpp"

#include <algorithm>

#include "Widget.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	halnf clampToExtent(halnf length, const Extent& extent) {
		if (length > extent.maximum) length = extent.maximum;
		if (length < extent.minimum) length = extent.minimum;
		return length;
	}

	Extent largestOf(const LayoutContext& context, Axis axis, halnf crossLength) {
		Extent result = { 0, 0, kUnbounded, 0 };
		for (const Widget* child : context.children()) {
			const Extent extent = context.measure(*child, axis, crossLength);
			if (extent.minimum > result.minimum) result.minimum = extent.minimum;
			if (extent.preferred > result.preferred) result.preferred = extent.preferred;
			if (extent.grow > result.grow) result.grow = extent.grow;
		}
		return result;
	}

	Vec2F sizeWithin(const LayoutContext& context, const Widget& child, Vec2F available) {
		const Extent horizontal = context.measure(child, Axis::Horizontal, available.y);
		const halnf width = horizontal.grow > 0 ? clampToExtent(available.x, horizontal) : horizontal.preferred;

		const Extent vertical = context.measure(child, Axis::Vertical, width);
		const halnf height = vertical.grow > 0 ? clampToExtent(available.y, vertical) : vertical.preferred;

		return { width, height };
	}
}

Extent StackLayout::measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const {
	return largestOf(context, axis, crossLength);
}

void StackLayout::arrangeChildren(LayoutContext& context, const RectF& contentArea) {
	for (Widget* child : context.children())
		context.place(*child, contentArea);
}

Extent FreeLayout::measureChildren(const LayoutContext& context, Axis axis, halnf) const {
	Extent result = { 0, 0, kUnbounded, 0 };
	for (const Widget* child : context.children()) {
		const halnf end = along(child->requestedPosition(), axis) + context.measure(*child, axis, kUnbounded).preferred;
		if (end > result.preferred) result.preferred = end;
	}
	return result;
}

void FreeLayout::arrangeChildren(LayoutContext& context, const RectF& contentArea) {
	for (Widget* child : context.children()) {
		const Vec2F position = contentArea.pos + child->requestedPosition();
		const Vec2F available = contentArea.size - child->requestedPosition();
		context.place(*child, { position, sizeWithin(context, *child, available) });
	}
}

GridLayout::GridLayout(ualni columns, halnf gap) :
	mColumns(columns > 0 ? columns : 1),
	mGap(gap) {}

void GridLayout::setColumns(ualni columns) {
	mColumns = columns > 0 ? columns : 1;
	invalidate();
}

void GridLayout::setGap(halnf gap) {
	mGap = gap;
	invalidate();
}

void GridLayout::setCellSize(std::optional<Vec2F> cellSize) {
	mCellSize = cellSize;
	invalidate();
}

Extent GridLayout::measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const {
	if (mCellSize) {
		const ualni lines = axis == Axis::Horizontal ? std::min<ualni>(mColumns, context.children().size()) : rowCount(context);
		const halnf cell = axis == Axis::Horizontal ? mCellSize->x : mCellSize->y;
		return Extent::rigid(lines == 0 ? 0 : cell * (halnf) lines + mGap * (halnf) (lines - 1));
	}

	if (axis == Axis::Horizontal) {
		const Extent widest = largestOf(context, Axis::Horizontal, kUnbounded);
		const halnf gaps = mGap * (halnf) (mColumns - 1);
		return { widest.minimum * (halnf) mColumns + gaps, widest.preferred * (halnf) mColumns + gaps, kUnbounded, 0 };
	}

	const halnf cellWidth = crossLength < kUnbounded ? columnWidth(crossLength) : kUnbounded;
	const ualni rows = rowCount(context);

	halnf height = rows > 1 ? mGap * (halnf) (rows - 1) : 0;
	for (ualni row = 0; row < rows; row++)
		height += rowHeight(context, row, cellWidth);
	return { height, height, kUnbounded, 0 };
}

void GridLayout::arrangeChildren(LayoutContext& context, const RectF& contentArea) {
	if (mCellSize) {
		for (ualni index = 0; index < context.children().size(); index++) {
			const halnf left = contentArea.pos.x + (mCellSize->x + mGap) * (halnf) (index % mColumns);
			const halnf top = contentArea.pos.y + (mCellSize->y + mGap) * (halnf) (index / mColumns);
			context.place(*context.children()[index], makeRect({ left, top }, *mCellSize));
		}
		return;
	}

	const halnf cellWidth = columnWidth(contentArea.size.x);
	halnf top = contentArea.pos.y;

	for (ualni row = 0; row < rowCount(context); row++) {
		const halnf height = rowHeight(context, row, cellWidth);

		for (ualni column = 0; column < mColumns; column++) {
			const ualni index = row * mColumns + column;
			if (index >= context.children().size()) break;

			const halnf left = contentArea.pos.x + (cellWidth + mGap) * (halnf) column;
			context.place(*context.children()[index], { left, top, cellWidth, height });
		}

		top += height + mGap;
	}
}

ualni GridLayout::rowCount(const LayoutContext& context) const {
	return (context.children().size() + mColumns - 1) / mColumns;
}

halnf GridLayout::columnWidth(halnf availableWidth) const {
	const halnf width = (availableWidth - mGap * (halnf) (mColumns - 1)) / (halnf) mColumns;
	return width > 0 ? width : 0;
}

halnf GridLayout::rowHeight(const LayoutContext& context, ualni row, halnf cellWidth) const {
	halnf height = 0;
	for (ualni column = 0; column < mColumns; column++) {
		const ualni index = row * mColumns + column;
		if (index >= context.children().size()) break;

		const halnf childHeight = context.measure(*context.children()[index], Axis::Vertical, cellWidth).preferred;
		if (childHeight > height) height = childHeight;
	}
	return height;
}
