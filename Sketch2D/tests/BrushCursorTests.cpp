#include "TestSketchesRoot.hpp"
#include "BrushCursor.hpp"
#include "FakeWindowHost.hpp"
#include "Session.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class CursorViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class CursorIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct CursorScene {
		Document document;
		Brush brush;
		CursorViewport viewport;
		CursorIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };

		CursorScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.setSize(40);
		}
	};

	Brush roundBrush(halnf size) {
		Brush brush;
		brush.setSize(size);
		return brush;
	}

	halnf farthest(const BrushCursorShape& shape) {
		halnf distance = 0;
		for (const Vec2F& point : shape.outline)
			distance = std::max(distance, (halnf) (point - shape.center).length());
		return distance;
	}

	halnf nearest(const BrushCursorShape& shape) {
		halnf distance = 1.0e9f;
		for (const Vec2F& point : shape.outline)
			distance = std::min(distance, (halnf) (point - shape.center).length());
		return distance;
	}

	size_t polylinesNear(RecordingPainter& painter, Vec2F center, halnf radius) {
		size_t count = 0;
		for (const PaintCommand& command : painter.commands()) {
			if (command.kind != PaintCommand::Kind::Polyline || command.points.empty()) continue;
			halnf far = 0;
			for (const Vec2F& point : command.points)
				far = std::max(far, (halnf) (point - center).length());
			if (std::fabs(far - radius) < 1.5f) count++;
		}
		return count;
	}

	size_t polygons(RecordingPainter& painter) {
		return (size_t) std::count_if(painter.commands().begin(), painter.commands().end(), [](const PaintCommand& c) {
			return c.kind == PaintCommand::Kind::Polygon;
		});
	}

	const MenuItem* menuItem(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items())
			if (item->text() == text) return item.get();
		return nullptr;
	}

	void activate(Menu& menu, const std::string& text) {
		const MenuItem* item = menuItem(menu, text);
		CHECK(item != nullptr);
		if (item) item->entry().action();
	}

	bool checked(Menu& menu, const std::string& text) {
		const MenuItem* item = menuItem(menu, text);
		return item && item->entry().isChecked && item->entry().isChecked();
	}
}

SUITE(BrushCursorShapes) {
	TEST(DefaultsMatchReference) {
		const BrushCursorSettings settings;
		CHECK(settings.style == BrushCursorStyle::Contour);
		CHECK(settings.contourForEraserAndSmudge);
		CHECK(settings.crosshairForSmallContour);
		CHECK(!settings.spotlight);
		CHECK(settings.showForPen);
		CHECK_CLOSE(0.4f, settings.spotlightAlpha, 1e-6f);
		CHECK_CLOSE(60.0f, settings.spotlightColor.hueDegrees, 1e-6f);
		CHECK_EQUAL(std::string("Contour"), brushCursorStyleName(BrushCursorStyle::Contour));
		CHECK_EQUAL(std::string("Brush"), brushCursorStyleName(BrushCursorStyle::Brush));
		CHECK_EQUAL(std::string("Arrow"), brushCursorStyleName(BrushCursorStyle::Arrow));
		CHECK_EQUAL(std::string("Cross"), brushCursorStyleName(BrushCursorStyle::Cross));
	}

	TEST(ContourFollowsBrushSizeAndZoom) {
		const BrushCursorShape shape = BrushCursorShape::build({}, roundBrush(40), { 100, 80 }, 2.0f, 0.0f);
		CHECK(shape.contour);
		CHECK(!shape.crosshair);
		CHECK_CLOSE(40.0f, shape.radius, 0.01f);
		CHECK_CLOSE(40.0f, farthest(shape), 0.05f);
		CHECK_CLOSE(40.0f, nearest(shape), 0.05f);
		CHECK_EQUAL((size_t) BrushCursorShape::kContourSegments + 1, shape.outline.size());
	}

	TEST(ContourIsAnEllipseRotatedWithTheBrushAndView) {
		Brush brush = roundBrush(100);
		brush.set(BrushParameter::Roundness, 0.5f);
		brush.set(BrushParameter::Angle, 90.0f);
		const BrushCursorShape upright = BrushCursorShape::build({}, brush, { 0, 0 }, 1.0f, 0.0f);
		CHECK_CLOSE(50.0f, farthest(upright), 0.05f);
		CHECK_CLOSE(brush.dabAt(1.0f).radius() * brush.dabAt(1.0f).roundness, nearest(upright), 0.5f);
		const Vec2F tip = upright.outline.front();
		CHECK_CLOSE(0.0f, tip.x, 0.05f);
		CHECK_CLOSE(50.0f, std::fabs(tip.y), 0.05f);

		const BrushCursorShape turned = BrushCursorShape::build({}, brush, { 0, 0 }, 1.0f, -1.5707963f);
		CHECK_CLOSE(50.0f, std::fabs(turned.outline.front().x), 0.05f);
	}

	TEST(SmallContoursAddACrosshair) {
		const BrushCursorShape small = BrushCursorShape::build({}, roundBrush(6), { 0, 0 }, 1.0f, 0.0f);
		CHECK(small.contour);
		CHECK(small.crosshair);

		const BrushCursorShape justLarge = BrushCursorShape::build({}, roundBrush(8.5f), { 0, 0 }, 1.0f, 0.0f);
		CHECK(!justLarge.crosshair);

		BrushCursorSettings off;
		off.crosshairForSmallContour = false;
		CHECK(!BrushCursorShape::build(off, roundBrush(6), { 0, 0 }, 1.0f, 0.0f).crosshair);
	}

	TEST(EraserAndSmudgeContourCanBeTurnedOff) {
		Brush eraser = roundBrush(30);
		eraser.setEraserMode(true);
		CHECK(BrushCursorShape::build({}, eraser, { 0, 0 }, 1.0f, 0.0f).contour);

		BrushCursorSettings settings;
		settings.contourForEraserAndSmudge = false;
		CHECK(!BrushCursorShape::build(settings, eraser, { 0, 0 }, 1.0f, 0.0f).contour);
		CHECK(BrushCursorShape::build(settings, roundBrush(30), { 0, 0 }, 1.0f, 0.0f).contour);
	}

	TEST(OtherStylesDrawNoContour) {
		for (const BrushCursorStyle style : { BrushCursorStyle::Brush, BrushCursorStyle::Arrow, BrushCursorStyle::Cross }) {
			BrushCursorSettings settings;
			settings.style = style;
			const BrushCursorShape shape = BrushCursorShape::build(settings, roundBrush(30), { 0, 0 }, 1.0f, 0.0f);
			CHECK(!shape.contour);
			CHECK(shape.outline.empty());
			CHECK_EQUAL(style == BrushCursorStyle::Cross, shape.crosshair);
		}
	}

	TEST(SpotlightOnlyWithAContour) {
		BrushCursorSettings settings;
		settings.spotlight = true;
		CHECK(BrushCursorShape::build(settings, roundBrush(30), { 0, 0 }, 1.0f, 0.0f).spotlight);
		settings.style = BrushCursorStyle::Arrow;
		CHECK(!BrushCursorShape::build(settings, roundBrush(30), { 0, 0 }, 1.0f, 0.0f).spotlight);
	}

	TEST(CursorSettingsSurviveASessionRoundTrip) {
		SessionSnapshot snapshot;
		BrushCursorSettings& cursor = snapshot.settings.cursor;
		cursor.style = BrushCursorStyle::Cross;
		cursor.contourForEraserAndSmudge = false;
		cursor.crosshairForSmallContour = false;
		cursor.spotlight = true;
		cursor.spotlightColor = { 200, 0.5f, 0.25f };
		cursor.spotlightAlpha = 0.7f;
		cursor.showForPen = false;

		const auto parsed = SessionSnapshot::parse(snapshot.serialize());
		REQUIRE CHECK(parsed.has_value());
		const BrushCursorSettings& loaded = parsed->settings.cursor;
		CHECK(loaded.style == BrushCursorStyle::Cross);
		CHECK(!loaded.contourForEraserAndSmudge);
		CHECK(!loaded.crosshairForSmallContour);
		CHECK(loaded.spotlight);
		CHECK_CLOSE(200.0f, loaded.spotlightColor.hueDegrees, 0.01f);
		CHECK_CLOSE(0.25f, loaded.spotlightColor.value, 0.01f);
		CHECK_CLOSE(0.7f, loaded.spotlightAlpha, 0.01f);
		CHECK(!loaded.showForPen);
		CHECK(!SessionSnapshot::parse("sketch2d-session 1\nview.cursor 9 1 1 0 60 1 1 0.4 1\n").has_value());
	}
}

SUITE(BrushCursorOnCanvas) {
	TEST(HoveringTheCanvasDrawsTheBrushContour) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F point = driver.centerOf(scene.root.canvas);
		driver.moveTo(point);
		CHECK(scene.root.canvas.isHoveringCanvas());

		RecordingPainter& painter = driver.paint();
		CHECK_EQUAL((size_t) 2, polylinesNear(painter, point, 20.0f * scene.document.zoom()));
		CHECK(scene.root.canvas.cursor() == CursorShape::Hidden);
	}

	TEST(ContourScalesWithZoom) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F point = driver.centerOf(scene.root.canvas);
		driver.moveTo(point);
		driver.wheel(1);
		driver.settle();
		driver.moveTo(point + Vec2F{ 1, 0 });
		const halnf radius = 20.0f * scene.document.zoom();
		CHECK(radius > 20.5f);
		CHECK_EQUAL((size_t) 2, polylinesNear(driver.paint(), point + Vec2F{ 1, 0 }, radius));
	}

	TEST(LeavingTheCanvasHidesTheContour) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.moveTo(driver.centerOf(scene.root.canvas));
		driver.moveTo(driver.centerOf(scene.root.menuBar));
		CHECK(!scene.root.canvas.isHoveringCanvas());
		CHECK(scene.root.canvas.brushCursor().outline.empty());
	}

	TEST(ViewCursorMenuSwitchesTheStyle) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		REQUIRE CHECK(menuItem(scene.root.viewMenu, "Cursor") != nullptr);

		Menu& menu = scene.root.cursorMenu;
		REQUIRE CHECK(menuItem(menu, "Brush") != nullptr);
		CHECK(checked(menu, "Contour"));

		const Vec2F point = driver.centerOf(scene.root.canvas);
		activate(menu, "Cross");
		driver.moveTo(point);
		CHECK(checked(menu, "Cross"));
		CHECK(!checked(menu, "Contour"));
		CHECK(scene.root.canvas.settings().cursor.style == BrushCursorStyle::Cross);
		CHECK(scene.root.canvas.cursor() == CursorShape::Crosshair);
		CHECK_EQUAL((size_t) 0, polylinesNear(driver.paint(), point, 20.0f * scene.document.zoom()));

		activate(menu, "Arrow");
		driver.moveTo(point + Vec2F{ 2, 0 });
		CHECK(scene.root.canvas.cursor() == CursorShape::Default);
	}

	TEST(SmallBrushShowsCrosshairAndSystemCrosshairWhenContourIsTiny) {
		CursorScene scene;
		scene.brush.setSize(4);
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F point = driver.centerOf(scene.root.canvas);
		driver.moveTo(point);
		CHECK(scene.root.canvas.brushCursor().crosshair);
		size_t lines = 0;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Line) lines++;
		CHECK(lines >= 4);

		activate(scene.root.cursorMenu, "Show crosshair for small contour");
		CHECK(!scene.root.canvas.settings().cursor.crosshairForSmallContour);
		driver.moveTo(point + Vec2F{ 1, 1 });
		CHECK(!scene.root.canvas.brushCursor().crosshair);
	}

	TEST(SpotlightFillsTheContour) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F point = driver.centerOf(scene.root.canvas);
		driver.moveTo(point);
		CHECK_EQUAL((size_t) 0, polygons(driver.paint()));

		activate(scene.root.cursorMenu, "Show contour spotlight");
		driver.moveTo(point + Vec2F{ 1, 0 });
		CHECK_EQUAL((size_t) 1, polygons(driver.paint()));
	}

	TEST(EraserContourFollowsTheSetting) {
		CursorScene scene;
		scene.brush.setEraserMode(true);
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F point = driver.centerOf(scene.root.canvas);
		driver.moveTo(point);
		CHECK(scene.root.canvas.brushCursor().contour);

		activate(scene.root.cursorMenu, "Show contour for eraser & smudge");
		driver.moveTo(point + Vec2F{ 1, 0 });
		CHECK(!scene.root.canvas.brushCursor().contour);
		CHECK(scene.root.canvas.cursor() == CursorShape::Crosshair);
	}

	TEST(PenCursorCanBeHidden) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setPointerTool(PointerTool::Pen);
		const Vec2F point = driver.centerOf(scene.root.canvas);
		driver.moveTo(point);
		CHECK(scene.root.canvas.brushCursor().contour);

		activate(scene.root.cursorMenu, "Show cursor for pen");
		driver.moveTo(point + Vec2F{ 1, 0 });
		CHECK(scene.root.canvas.brushCursor().outline.empty());
		CHECK(scene.root.canvas.cursor() == CursorShape::Hidden);

		driver.setPointerTool(PointerTool::Mouse);
		driver.moveTo(point + Vec2F{ 2, 0 });
		CHECK(scene.root.canvas.brushCursor().contour);
	}

	TEST(TheWindowGetsTheCanvasCursor) {
		CursorScene scene;
		FakeWindowHost host;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.ui().setWindowHost(&host);
		driver.moveTo(driver.centerOf(scene.root.canvas));
		REQUIRE CHECK(!host.cursors.empty());
		CHECK(host.cursors.back() == CursorShape::Hidden);

		activate(scene.root.cursorMenu, "Cross");
		driver.moveTo(driver.centerOf(scene.root.canvas) + Vec2F{ 3, 0 });
		CHECK(host.cursors.back() == CursorShape::Crosshair);

		driver.moveTo(driver.centerOf(scene.root.menuBar));
		CHECK(host.cursors.back() == CursorShape::Default);
		driver.ui().setWindowHost(nullptr);
	}

	TEST(PanningShowsTheGrabbingHand) {
		CursorScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.moveTo(driver.centerOf(scene.root.canvas));
		driver.press(PointerButton::Middle);
		driver.moveTo(driver.centerOf(scene.root.canvas) + Vec2F{ 30, 0 });
		CHECK(scene.root.canvas.isPanning());
		CHECK(scene.root.canvas.cursor() == CursorShape::Grabbing);
		CHECK(scene.root.canvas.brushCursor().outline.empty());
		driver.release(PointerButton::Middle);
	}
}
