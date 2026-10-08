#include "TestSketchesRoot.hpp"
#include <algorithm>
#include "Selection.hpp"
#include "Sketch2DUI.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class SelectViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class SelectIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct SelectScene {
		Document document;
		Brush brush;
		SelectViewport viewport;
		SelectIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		SelectScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
			brush.setSize(60);
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		halnf alphaAt(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false).a; }
	};
}

SUITE(Selection) {
	TEST(RectangleCoversExactlyItsPixels) {
		Selection selection;
		CHECK(!selection.isActive());
		CHECK_EQUAL(255, selection.coverage(-1000, 5000));
		selection.addRectangle(10, 20, 30, 40, SelectionMode::Replace);
		CHECK(selection.isActive());
		CHECK_EQUAL(255, selection.coverage(10, 20));
		CHECK_EQUAL(255, selection.coverage(29, 39));
		CHECK_EQUAL(0, selection.coverage(30, 39));
		CHECK_EQUAL(0, selection.coverage(9, 25));
		int4 left = 0, top = 0, right = 0, bottom = 0;
		REQUIRE CHECK(selection.bounds(left, top, right, bottom));
		CHECK_EQUAL(10, left);
		CHECK_EQUAL(20, top);
		CHECK_EQUAL(30, right);
		CHECK_EQUAL(40, bottom);
	}

	TEST(AddSubtractAndReplaceCombine) {
		Selection selection;
		selection.addRectangle(0, 0, 10, 10, SelectionMode::Replace);
		selection.addRectangle(20, 0, 30, 10, SelectionMode::Add);
		CHECK_EQUAL(255, selection.coverage(5, 5));
		CHECK_EQUAL(255, selection.coverage(25, 5));
		selection.addRectangle(0, 0, 5, 10, SelectionMode::Subtract);
		CHECK_EQUAL(0, selection.coverage(2, 5));
		CHECK_EQUAL(255, selection.coverage(7, 5));
		selection.addRectangle(100, 100, 110, 110, SelectionMode::Replace);
		CHECK_EQUAL(0, selection.coverage(25, 5));
		CHECK_EQUAL(255, selection.coverage(105, 105));
		selection.addRectangle(100, 100, 110, 110, SelectionMode::Subtract);
		CHECK(!selection.isActive());
	}

	TEST(SubtractWithoutASelectionSelectsNothing) {
		Selection selection;
		selection.addRectangle(0, 0, 10, 10, SelectionMode::Subtract);
		CHECK(!selection.isActive());
	}

	TEST(EllipseIsAntialiased) {
		Selection selection;
		selection.addEllipse({ 50, 50 }, { 20, 10 }, SelectionMode::Replace);
		CHECK_EQUAL(255, selection.coverage(50, 50));
		CHECK_EQUAL(255, selection.coverage(68, 50));
		CHECK_EQUAL(0, selection.coverage(71, 50));
		CHECK_EQUAL(0, selection.coverage(32, 42));
		bool partial = false;
		for (int4 x = 30; x < 70; x++)
			for (int4 y = 40; y < 60; y++) {
				const uint1 value = selection.coverage(x, y);
				if (value > 0 && value < 255) partial = true;
			}
		CHECK(partial);
	}

	TEST(PolygonFollowsItsEdges) {
		Selection selection;
		selection.addPolygon({ { 0, 0 }, { 40, 0 }, { 0, 40 } }, SelectionMode::Replace);
		CHECK_EQUAL(255, selection.coverage(5, 5));
		CHECK_EQUAL(0, selection.coverage(30, 30));
		CHECK_EQUAL(255, selection.coverage(18, 18));
		CHECK_EQUAL(0, selection.coverage(25, 25));
	}

	TEST(InvertFlipsCoverageEverywhere) {
		Selection selection;
		selection.addRectangle(0, 0, 10, 10, SelectionMode::Replace);
		selection.invert();
		CHECK(selection.isInverted());
		CHECK_EQUAL(0, selection.coverage(5, 5));
		CHECK_EQUAL(255, selection.coverage(500, -500));
		selection.addRectangle(100, 100, 110, 110, SelectionMode::Subtract);
		CHECK_EQUAL(0, selection.coverage(105, 105));
		CHECK_EQUAL(255, selection.coverage(120, 105));
		selection.addRectangle(0, 0, 5, 5, SelectionMode::Add);
		CHECK_EQUAL(255, selection.coverage(2, 2));
		CHECK_EQUAL(0, selection.coverage(7, 7));
	}

	TEST(OutlineTracesTheBorder) {
		Selection selection;
		selection.addRectangle(0, 0, 3, 2, SelectionMode::Replace);
		CHECK_EQUAL((size_t) 10, selection.outline().size());
	}

	TEST(PaintingStaysInsideTheSelection) {
		SelectScene scene;
		scene.document.editSelection().addRectangle(0, 0, 200, 200, SelectionMode::Replace);
		scene.driver.drag(scene.screenOf({ 150, 100 }), scene.screenOf({ 260, 100 }));
		scene.document.settlePaint();
		CHECK(scene.alphaAt(180, 100) > 0.5f);
		CHECK_CLOSE(0.0f, scene.alphaAt(230, 100), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(205, 100), 1e-6f);
	}

	TEST(BucketAndGradientRespectTheSelection) {
		SelectScene scene;
		Layer& layer = *scene.document.activeLayer();
		for (int4 y = 0; y < 40; y++)
			for (int4 x = 0; x < 40; x++) {
				const bool wall = x < 2 || y < 2 || x >= 38 || y >= 38;
				if (!wall) continue;
				uint1* pixel = layer.tileAt({ 0, 0 })->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
				pixel[3] = 255;
			}
		scene.document.editSelection().addRectangle(0, 0, 20, 40, SelectionMode::Replace);
		BucketSettings bucket;
		bucket.spread = 0;
		CHECK(scene.document.bucketFill({ 10, 10 }, { 1, 0, 0, 1 }, bucket) == BucketResult::Filled);
		CHECK_CLOSE(1.0f, scene.alphaAt(10, 10), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(30, 10), 1e-6f);

		Document framed;
		framed.editSelection().addRectangle(0, 0, 50, 50, SelectionMode::Replace);
		CHECK(
			framed.drawGradient({ 0, 0 }, { 50, 0 }, { 1, 0, 0, 1 }, { 0, 0, 1, 1 }, GradientSettings()) ==
			GradientResult::Drawn
		);
		CHECK_CLOSE(1.0f, framed.sampleColor({ 10.5f, 10.5f }, false).a, 1e-6f);
		CHECK_CLOSE(0.0f, framed.sampleColor({ 60.5f, 10.5f }, false).a, 1e-6f);
	}

	TEST(RectangleToolSelectsAndUndoes) {
		SelectScene scene;
		scene.root.commands.run(Command::ToolRectangleSelect);
		CHECK(scene.root.canvas.tool() == CanvasTool::Marquee);
		CHECK(scene.root.toolStrip.button("rectselect").isSelected());
		const size_t revision = scene.document.history().revision();
		scene.driver.drag(scene.screenOf({ 50, 50 }), scene.screenOf({ 150, 120 }));
		CHECK(scene.document.hasSelection());
		CHECK_EQUAL(255, scene.document.selection().coverage(100, 100));
		CHECK_EQUAL(0, scene.document.selection().coverage(160, 100));
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		scene.root.commands.run(Command::Undo);
		CHECK(!scene.document.hasSelection());
		scene.root.commands.run(Command::Redo);
		CHECK(scene.document.hasSelection());
	}

	TEST(ShiftAddsAndAltSubtracts) {
		SelectScene scene;
		scene.root.commands.run(Command::ToolRectangleSelect);
		scene.driver.drag(scene.screenOf({ 50, 50 }), scene.screenOf({ 100, 100 }));
		scene.driver.moveTo(scene.screenOf({ 200, 50 }));
		scene.driver.press(PointerButton::Primary, Modifiers{ true, false, false });
		scene.driver.moveTo(scene.screenOf({ 250, 100 }));
		scene.driver.release();
		CHECK_EQUAL(255, scene.document.selection().coverage(75, 75));
		CHECK_EQUAL(255, scene.document.selection().coverage(225, 75));
		scene.driver.moveTo(scene.screenOf({ 40, 40 }));
		scene.driver.press(PointerButton::Primary, Modifiers{ false, false, true });
		scene.driver.moveTo(scene.screenOf({ 75, 110 }));
		scene.driver.release();
		CHECK_EQUAL(0, scene.document.selection().coverage(60, 75));
		CHECK_EQUAL(255, scene.document.selection().coverage(90, 75));
	}

	TEST(EllipseOptionSelectsAnEllipse) {
		SelectScene scene;
		scene.root.canvas.settings().selection.marquee = MarqueeShape::Ellipse;
		scene.root.commands.run(Command::ToolRectangleSelect);
		scene.driver.drag(scene.screenOf({ 0, 0 }), scene.screenOf({ 100, 100 }));
		CHECK_EQUAL(255, scene.document.selection().coverage(50, 50));
		CHECK_EQUAL(0, scene.document.selection().coverage(3, 3));
	}

	TEST(FreehandLassoFollowsThePath) {
		SelectScene scene;
		scene.root.commands.run(Command::ToolLasso);
		CHECK(scene.root.canvas.tool() == CanvasTool::Lasso);
		scene.driver.moveTo(scene.screenOf({ 0, 0 }));
		scene.driver.press();
		scene.driver.moveTo(scene.screenOf({ 100, 0 }));
		scene.driver.moveTo(scene.screenOf({ 0, 100 }));
		scene.driver.release();
		CHECK_EQUAL(255, scene.document.selection().coverage(20, 20));
		CHECK_EQUAL(0, scene.document.selection().coverage(80, 80));
	}

	TEST(PolygonLassoClosesOnTheFirstPointOrEnter) {
		SelectScene scene;
		scene.root.canvas.settings().selection.lasso = LassoShape::Polygon;
		scene.root.commands.run(Command::ToolLasso);
		scene.driver.click(scene.screenOf({ 0, 0 }));
		scene.driver.click(scene.screenOf({ 100, 0 }));
		scene.driver.click(scene.screenOf({ 100, 100 }));
		CHECK_EQUAL((size_t) 3, scene.root.canvas.polygonPoints().size());
		CHECK(!scene.document.hasSelection());
		scene.driver.click(scene.screenOf({ 2, 2 }));
		CHECK(scene.root.canvas.polygonPoints().empty());
		CHECK_EQUAL(255, scene.document.selection().coverage(80, 20));
		CHECK_EQUAL(0, scene.document.selection().coverage(20, 80));

		scene.root.commands.run(Command::Deselect);
		scene.driver.click(scene.screenOf({ 200, 0 }));
		scene.driver.click(scene.screenOf({ 300, 0 }));
		scene.driver.click(scene.screenOf({ 300, 100 }));
		scene.driver.key(InputID::ENTER);
		CHECK_EQUAL(255, scene.document.selection().coverage(280, 20));
	}

	TEST(ClickDeselectsWhenEnabled) {
		SelectScene scene;
		scene.root.commands.run(Command::ToolRectangleSelect);
		scene.driver.drag(scene.screenOf({ 50, 50 }), scene.screenOf({ 100, 100 }));
		scene.root.canvas.settings().selection.clickToDeselect = false;
		scene.driver.click(scene.screenOf({ 300, 300 }));
		CHECK(scene.document.hasSelection());
		scene.root.canvas.settings().selection.clickToDeselect = true;
		scene.driver.click(scene.screenOf({ 300, 300 }));
		CHECK(!scene.document.hasSelection());
	}

	TEST(SelectAllInvertDeselectAndEscape) {
		SelectScene scene;
		scene.driver.drag(scene.screenOf({ 100, 100 }), scene.screenOf({ 200, 100 }));
		scene.document.settlePaint();
		scene.root.commands.run(Command::SelectAllAsRectangle);
		CHECK(scene.document.hasSelection());
		CHECK_EQUAL(255, scene.document.selection().coverage(150, 100));
		scene.root.commands.run(Command::InvertSelection);
		CHECK(scene.document.selection().isInverted());
		CHECK_EQUAL(0, scene.document.selection().coverage(150, 100));
		scene.driver.key(InputID::ESCAPE);
		CHECK(!scene.document.hasSelection());
		scene.root.commands.run(Command::Undo);
		CHECK(scene.document.selection().isInverted());
	}

	TEST(SelectAllHugsTheActiveLayer) {
		SelectScene scene;
		Layer& layer = *scene.document.activeLayer();
		for (int4 y = 10; y < 30; y++)
			for (int4 x = 10; x < 30; x++) {
				const bool corner = x < 15 && y < 15;
				uint1* pixel = layer.tileAt({ 0, 0 })->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
				pixel[3] = corner ? 0 : (x == 29 ? 128 : 255);
			}
		Layer& other = *scene.document.addLayer("Other");
		uint1* far = other.tileAt({ 2, 2 })->pixels();
		far[3] = 255;
		scene.document.setActiveLayer(&layer);

		scene.root.commands.run(Command::SelectAllAsRectangle);
		int4 left = 0, top = 0, right = 0, bottom = 0;
		REQUIRE CHECK(scene.document.selection().bounds(left, top, right, bottom));
		CHECK_EQUAL(10, left);
		CHECK_EQUAL(10, top);
		CHECK_EQUAL(30, right);
		CHECK_EQUAL(30, bottom);
		CHECK_EQUAL(255, scene.document.selection().coverage(11, 11));

		scene.root.commands.run(Command::SelectAllAsLasso);
		CHECK_EQUAL(0, scene.document.selection().coverage(11, 11));
		CHECK_EQUAL(255, scene.document.selection().coverage(20, 20));
		CHECK_EQUAL(128, scene.document.selection().coverage(29, 20));
		CHECK_EQUAL(0, scene.document.selection().coverage(31, 20));

		scene.root.commands.run(Command::Undo);
		CHECK_EQUAL(255, scene.document.selection().coverage(11, 11));
		scene.document.setActiveLayer(&other);
		scene.root.commands.run(Command::Deselect);
		scene.document.addLayer("Empty");
		scene.root.commands.run(Command::SelectAllAsLasso);
		CHECK_EQUAL(std::string("Nothing to select"), scene.root.lastNotice());
	}

	TEST(SelectCanvasFrameUsesTheFrame) {
		SelectScene scene;
		scene.root.commands.run(Command::SelectCanvasFrame);
		CHECK(!scene.document.hasSelection());
		scene.document.setInfinite(false);
		scene.document.setFrame(0, 0, 300, 200);
		scene.root.commands.run(Command::SelectCanvasFrame);
		int4 left = 0, top = 0, right = 0, bottom = 0;
		REQUIRE CHECK(scene.document.selection().bounds(left, top, right, bottom));
		CHECK_EQUAL(300, right);
		CHECK_EQUAL(200, bottom);
	}

	TEST(MarchingAntsAreDrawn) {
		SelectScene scene;
		scene.document.editSelection().addRectangle(10, 10, 20, 20, SelectionMode::Replace);
		scene.root.canvas.requestPaint();
		scene.driver.settle();
		size_t dark = 0;
		size_t light = 0;
		for (const PaintCommand& command : scene.driver.paint().commands()) {
			if (command.kind != PaintCommand::Kind::Line) continue;
			if (command.color.r < 0.1f && command.color.a > 0.8f) dark++;
			if (command.color.r > 0.9f && command.color.a > 0.8f) light++;
		}
		CHECK_EQUAL((size_t) 40, dark);
		CHECK(light > 0);
		CHECK(light < dark);
	}

	TEST(SelectMenuAndToolbarButton) {
		SelectScene scene;
		scene.driver.click(scene.root.toolBar.select);
		CHECK(scene.root.selectMenu.isOpen());
		std::vector<std::string> texts;
		for (const auto& item : scene.root.selectMenu.items())
			texts.push_back(item->entry().text);
		for (const char* text : { "Select all as rectangle",
															"Select canvas frame",
															"Invert selection",
															"Deselect",
															"Rectangle selection tool",
															"Lasso selection tool",
															"Ellipse",
															"Polygon lasso" })
			CHECK(std::find(texts.begin(), texts.end(), text) != texts.end());
	}

	TEST(TabletQuickMenuHasTheSelectMenu) {
		Document document;
		Brush brush;
		SelectViewport viewport;
		SelectIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		bool found = false;
		for (const auto& item : classic.mainMenu.items())
			if (item->entry().text == "Select" && item->entry().submenu == &classic.selectMenu) found = true;
		CHECK(found);
	}

	TEST(TabletToolsMenuHasSelectionTools) {
		Document document;
		Brush brush;
		SelectViewport viewport;
		SelectIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		studio.useTool("lasso");
		CHECK(classic.canvas.tool() == CanvasTool::Lasso);
	}
}
