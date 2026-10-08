#include "WindowFrame.hpp"

#include <algorithm>

using namespace tp;

CursorShape tp::cursorForEdge(WindowEdge edge) {
	switch (edge) {
		case WindowEdge::Left:
		case WindowEdge::Right: return CursorShape::ResizeHorizontal;
		case WindowEdge::Top:
		case WindowEdge::Bottom: return CursorShape::ResizeVertical;
		case WindowEdge::TopLeft:
		case WindowEdge::BottomRight: return CursorShape::ResizeNorthWestSouthEast;
		case WindowEdge::TopRight:
		case WindowEdge::BottomLeft: return CursorShape::ResizeNorthEastSouthWest;
		case WindowEdge::None: return CursorShape::Default;
	}
	return CursorShape::Default;
}

Vec2F WindowFrame::surfaceSize() const {
	const halnf inset = margin();
	return { mContentSize.x + inset * 2, mContentSize.y + inset * 2 };
}

RectF WindowFrame::contentRect() const {
	const halnf inset = margin();
	return { inset, inset, mContentSize.x, mContentSize.y };
}

RectF WindowFrame::inputRect() const {
	const RectF content = contentRect();
	const halnf outer = outerResizeBorder();
	return { content.x - outer, content.y - outer, content.z + outer * 2, content.w + outer * 2 };
}

halnf WindowFrame::outerResizeBorder() const { return std::min(kResizeBorder, margin()); }

halnf WindowFrame::innerResizeBorder() const {
	return std::max(kMinimumInnerResizeBorder, kResizeBorder - outerResizeBorder());
}

WindowEdge WindowFrame::edgeAt(Vec2F point) const {
	if (mTiled) return WindowEdge::None;

	const RectF zone = inputRect();
	const bool insideZone =
		point.x >= zone.x && point.y >= zone.y && point.x < zone.x + zone.z && point.y < zone.y + zone.w;
	if (!insideZone) return WindowEdge::None;

	const RectF content = contentRect();
	const halnf inner = innerResizeBorder();

	const bool onLeft = point.x < content.x + inner;
	const bool onRight = point.x >= content.x + content.z - inner;
	const bool onTop = point.y < content.y + inner;
	const bool onBottom = point.y >= content.y + content.w - inner;

	const bool nearLeft = point.x < content.x + kCornerResizeLength;
	const bool nearRight = point.x >= content.x + content.z - kCornerResizeLength;
	const bool nearTop = point.y < content.y + kCornerResizeLength;
	const bool nearBottom = point.y >= content.y + content.w - kCornerResizeLength;

	if ((onTop && nearLeft) || (onLeft && nearTop)) return WindowEdge::TopLeft;
	if ((onTop && nearRight) || (onRight && nearTop)) return WindowEdge::TopRight;
	if ((onBottom && nearLeft) || (onLeft && nearBottom)) return WindowEdge::BottomLeft;
	if ((onBottom && nearRight) || (onRight && nearBottom)) return WindowEdge::BottomRight;
	if (onTop) return WindowEdge::Top;
	if (onBottom) return WindowEdge::Bottom;
	if (onLeft) return WindowEdge::Left;
	if (onRight) return WindowEdge::Right;
	return WindowEdge::None;
}
