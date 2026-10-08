#include "UnitTest++/UnitTest++.h"

#include "WindowFrame.hpp"

using namespace tp;

namespace {
	WindowFrame floatingFrame() {
		WindowFrame frame;
		frame.setContentSize({ 800, 600 });
		return frame;
	}
}

SUITE(WindowFrameGeometry) {
	TEST(FloatingTranslucentFrameReservesShadowMargin) {
		const WindowFrame frame = floatingFrame();
		const halnf margin = WindowFrame::kShadowMargin;

		CHECK(frame.hasShadow());
		CHECK_CLOSE(800 + margin * 2, frame.surfaceSize().x, 0.001f);
		CHECK_CLOSE(600 + margin * 2, frame.surfaceSize().y, 0.001f);
		CHECK_CLOSE(margin, frame.contentRect().x, 0.001f);
		CHECK_CLOSE(WindowFrame::kCornerRadius, frame.cornerRadius(), 0.001f);
	}

	TEST(TiledFrameDropsShadowAndCorners) {
		WindowFrame frame = floatingFrame();
		frame.setTiled(true);

		CHECK(!frame.hasShadow());
		CHECK_CLOSE(0, frame.margin(), 0.001f);
		CHECK_CLOSE(0, frame.cornerRadius(), 0.001f);
		CHECK_CLOSE(800, frame.surfaceSize().x, 0.001f);
	}

	TEST(OpaqueSurfaceDropsShadowButKeepsResizeEdges) {
		WindowFrame frame = floatingFrame();
		frame.setTranslucent(false);

		CHECK(!frame.hasShadow());
		CHECK(frame.edgeAt({ 1, 300 }) == WindowEdge::Left);
		CHECK(frame.edgeAt({ 400, 300 }) == WindowEdge::None);
	}

	TEST(InputRegionCoversContentPlusOuterResizeBorder) {
		const WindowFrame frame = floatingFrame();
		const RectF input = frame.inputRect();

		CHECK_CLOSE(WindowFrame::kShadowMargin - WindowFrame::kResizeBorder, input.x, 0.001f);
		CHECK_CLOSE(800 + WindowFrame::kResizeBorder * 2, input.z, 0.001f);
	}
}

SUITE(WindowFrameEdges) {
	TEST(ShadowOutsideResizeBorderIsNotInteractive) {
		const WindowFrame frame = floatingFrame();
		CHECK(frame.edgeAt({ 1, 1 }) == WindowEdge::None);
	}

	TEST(SidesResolveToSingleEdges) {
		const WindowFrame frame = floatingFrame();
		const RectF content = frame.contentRect();
		const Vec2F middle = { content.x + content.z / 2, content.y + content.w / 2 };

		CHECK(frame.edgeAt({ content.x - 2, middle.y }) == WindowEdge::Left);
		CHECK(frame.edgeAt({ content.x + content.z + 2, middle.y }) == WindowEdge::Right);
		CHECK(frame.edgeAt({ middle.x, content.y - 2 }) == WindowEdge::Top);
		CHECK(frame.edgeAt({ middle.x, content.y + content.w + 2 }) == WindowEdge::Bottom);
	}

	TEST(InnerResizeStripStaysThin) {
		const WindowFrame frame = floatingFrame();
		const RectF content = frame.contentRect();

		CHECK(frame.edgeAt({ content.x + 1, content.y + 200 }) == WindowEdge::Left);
		CHECK(frame.edgeAt({ content.x + 6, content.y + 200 }) == WindowEdge::None);
	}

	TEST(CornersWinOverSides) {
		const WindowFrame frame = floatingFrame();
		const RectF content = frame.contentRect();

		CHECK(frame.edgeAt({ content.x - 2, content.y + 4 }) == WindowEdge::TopLeft);
		CHECK(frame.edgeAt({ content.x + 4, content.y - 2 }) == WindowEdge::TopLeft);
		CHECK(frame.edgeAt({ content.x + content.z + 2, content.y + content.w - 4 }) == WindowEdge::BottomRight);
		CHECK(frame.edgeAt({ content.x + content.z - 4, content.y - 2 }) == WindowEdge::TopRight);
		CHECK(frame.edgeAt({ content.x - 2, content.y + content.w - 4 }) == WindowEdge::BottomLeft);
	}

	TEST(TiledFrameHasNoResizeEdges) {
		WindowFrame frame = floatingFrame();
		frame.setTiled(true);

		CHECK(frame.edgeAt({ 0, 0 }) == WindowEdge::None);
		CHECK(frame.edgeAt({ 1, 300 }) == WindowEdge::None);
	}

	TEST(EdgeCursorsMatchAxes) {
		CHECK(cursorForEdge(WindowEdge::Left) == CursorShape::ResizeHorizontal);
		CHECK(cursorForEdge(WindowEdge::Bottom) == CursorShape::ResizeVertical);
		CHECK(cursorForEdge(WindowEdge::TopLeft) == CursorShape::ResizeNorthWestSouthEast);
		CHECK(cursorForEdge(WindowEdge::BottomLeft) == CursorShape::ResizeNorthEastSouthWest);
		CHECK(cursorForEdge(WindowEdge::None) == CursorShape::Default);
	}
}
