#include "TestSketchesRoot.hpp"
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
	class MoveViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class MoveIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	void fillSquare(Document& document, int4 left, int4 top, int4 side, const RGBA& color) {
		Layer& layer = *document.activeLayer();
		for (int4 y = top; y < top + side; y++)
			for (int4 x = left; x < left + side; x++) {
				const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
				Tile* tile = layer.tileAt(coord);
				uint1* pixel =
					tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
				pixel[0] = (uint1) (color.r * 255);
				pixel[1] = (uint1) (color.g * 255);
				pixel[2] = (uint1) (color.b * 255);
				pixel[3] = 255;
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
			}
	}

	struct MoveScene {
		Document document;
		Brush brush;
		MoveViewport viewport;
		MoveIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		MoveScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
			fillSquare(document, 10, 10, 20, { 1, 0, 0, 1 });
			document.commit("Square");
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		halnf alphaAt(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false).a; }
	};
}

SUITE(MoveTool) {
	TEST(MovesPixelsAcrossTileEdges) {
		MoveScene scene;
		REQUIRE CHECK(scene.document.beginMove(false));
		scene.document.moveTo(kTileSize - 15, 5);
		CHECK(scene.document.finishMove());
		CHECK_CLOSE(0.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(kTileSize - 5, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(kTileSize + 10, 30), 1e-6f);
		CHECK_CLOSE(1.0f, scene.document.sampleColor(Vec2F{ (halnf) kTileSize + 1.0f, 20.0f }, false).r, 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(kTileSize + 20, 30), 1e-6f);
	}

	TEST(NegativeOffsetsWork) {
		MoveScene scene;
		scene.document.beginMove(false);
		scene.document.moveTo(-40, -40);
		scene.document.finishMove();
		CHECK_CLOSE(1.0f, scene.alphaAt(-25, -25), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(15, 15), 1e-6f);
	}

	TEST(DuplicateKeepsTheOriginal) {
		MoveScene scene;
		scene.document.beginMove(true);
		scene.document.moveTo(30, 0);
		scene.document.finishMove();
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(45, 15), 1e-6f);
	}

	TEST(DuplicatingThroughSeveralPositionsLeavesNoTrail) {
		MoveScene scene;
		scene.document.beginMove(true);
		scene.document.moveTo(30, 0);
		scene.document.moveTo(60, 0);
		scene.document.finishMove();
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(45, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(75, 15), 1e-6f);
	}

	TEST(PixelsCrossingATileRowStayWhole) {
		MoveScene scene;
		scene.document.beginMove(false);
		scene.document.moveTo(0, kTileSize - 20);
		scene.document.finishMove();
		CHECK_CLOSE(1.0f, scene.alphaAt(15, kTileSize - 5), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(15, kTileSize + 5), 1e-6f);
	}

	TEST(MovingBackToTheStartChangesNothing) {
		MoveScene scene;
		scene.document.beginMove(false);
		scene.document.moveTo(30, 0);
		scene.document.moveTo(0, 0);
		CHECK(!scene.document.finishMove());
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(45, 15), 1e-6f);
	}

	TEST(CancelRestoresTheLayer) {
		MoveScene scene;
		scene.document.beginMove(false);
		scene.document.moveTo(30, 30);
		scene.document.cancelMove();
		CHECK(!scene.document.isMoving());
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(45, 45), 1e-6f);
	}

	TEST(OtherLayersStayPut) {
		MoveScene scene;
		Layer& bottom = *scene.document.activeLayer();
		scene.document.addLayer("Top");
		fillSquare(scene.document, 100, 100, 10, { 0, 0, 1, 1 });
		scene.document.beginMove(false);
		scene.document.moveTo(20, 0);
		scene.document.finishMove();
		scene.document.setActiveLayer(&bottom);
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(125, 105), 1e-6f);
	}

	TEST(LockedOrEmptyLayersDoNotMove) {
		MoveScene scene;
		scene.document.toggleLock();
		CHECK(!scene.document.beginMove(false));
		scene.document.toggleLock();
		scene.document.addLayer("Empty");
		CHECK(!scene.document.beginMove(false));
	}

	TEST(ToolDragsPixelsAsOneUndoStep) {
		MoveScene scene;
		scene.root.commands.run(Command::ToolMove);
		CHECK(scene.root.canvas.tool() == CanvasTool::Move);
		CHECK(scene.root.toolStrip.button("move").isSelected());
		const size_t revision = scene.document.history().revision();
		scene.driver.moveTo(scene.screenOf({ 20, 20 }));
		scene.driver.press();
		scene.driver.moveTo(scene.screenOf({ 50, 20 }));
		CHECK(scene.root.canvas.isMovingPixels());
		CHECK_CLOSE(1.0f, scene.alphaAt(45, 15), 1e-6f);
		scene.driver.moveTo(scene.screenOf({ 70, 40 }));
		scene.driver.release();
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK_CLOSE(1.0f, scene.alphaAt(65, 35), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(45, 15), 1e-6f);
		scene.root.commands.run(Command::Undo);
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(65, 35), 1e-6f);
	}

	TEST(AltDragOrTheMenuDuplicates) {
		MoveScene scene;
		scene.root.commands.run(Command::ToolMove);
		scene.driver.moveTo(scene.screenOf({ 20, 20 }));
		scene.driver.press(PointerButton::Primary, Modifiers{ false, false, true });
		scene.driver.moveTo(scene.screenOf({ 60, 20 }));
		scene.driver.release();
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(55, 15), 1e-6f);

		MenuItem* duplicate = nullptr;
		for (const auto& item : scene.root.moveMenu.items())
			if (item->entry().text == "Duplicate pixels") duplicate = item.get();
		REQUIRE CHECK(duplicate != nullptr);
		duplicate->entry().action();
		CHECK(scene.root.canvas.settings().move.duplicate);
		CHECK(duplicate->entry().isChecked());
		scene.driver.drag(scene.screenOf({ 20, 20 }), scene.screenOf({ 20, 60 }));
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 55), 1e-6f);
	}

	TEST(ClickWithoutDraggingAddsNoUndoStep) {
		MoveScene scene;
		scene.root.commands.run(Command::ToolMove);
		const size_t revision = scene.document.history().revision();
		scene.driver.click(scene.screenOf({ 20, 20 }));
		CHECK_EQUAL(revision, scene.document.history().revision());
		CHECK(!scene.document.isMoving());
	}

	TEST(EmptyLayerExplains) {
		MoveScene scene;
		scene.document.addLayer("Empty");
		scene.root.commands.run(Command::ToolMove);
		scene.driver.drag(scene.screenOf({ 20, 20 }), scene.screenOf({ 60, 20 }));
		CHECK_EQUAL(std::string("Nothing to move on this layer"), scene.root.lastNotice());
	}

	TEST(OnlyTheSelectionMovesAndTheSelectionFollows) {
		MoveScene scene;
		scene.document.editSelection().addRectangle(10, 10, 20, 30, SelectionMode::Replace);
		scene.document.beginMove(false);
		scene.document.moveTo(40, 0);
		CHECK(scene.document.finishMove());
		CHECK_CLOSE(0.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(25, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(55, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(65, 15), 1e-6f);
		CHECK_EQUAL(255, scene.document.selection().coverage(55, 15));
		CHECK_EQUAL(0, scene.document.selection().coverage(15, 15));
	}

	TEST(DuplicatingASelectionKeepsTheOriginal) {
		MoveScene scene;
		scene.document.editSelection().addRectangle(10, 10, 20, 30, SelectionMode::Replace);
		scene.document.beginMove(true);
		scene.document.moveTo(40, 0);
		scene.document.finishMove();
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(55, 15), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(65, 15), 1e-6f);
	}

	TEST(SoftSelectionsMovePartially) {
		MoveScene scene;
		scene.document.editSelection().addRectangle(10, 10, 30, 30, SelectionMode::Replace);
		scene.document.editSelection().blur(2);
		const halnf take = scene.document.selection().coverage(10, 20) / 255.0f;
		scene.document.beginMove(false);
		scene.document.moveTo(0, 60);
		scene.document.finishMove();
		CHECK_CLOSE(1.0f - take, scene.alphaAt(10, 20), 0.02f);
		CHECK_CLOSE(take, scene.alphaAt(10, 80), 0.02f);
	}

	TEST(CancellingAMoveRestoresTheSelection) {
		MoveScene scene;
		scene.document.editSelection().addRectangle(10, 10, 20, 30, SelectionMode::Replace);
		scene.document.beginMove(false);
		scene.document.moveTo(40, 0);
		scene.document.cancelMove();
		CHECK_EQUAL(255, scene.document.selection().coverage(15, 15));
		CHECK_EQUAL(0, scene.document.selection().coverage(55, 15));
	}

	TEST(UndoBringsBackTheSelectionPosition) {
		MoveScene scene;
		scene.document.editSelection().addRectangle(10, 10, 20, 30, SelectionMode::Replace);
		scene.document.commit("Select");
		scene.root.commands.run(Command::ToolMove);
		scene.driver.drag(scene.screenOf({ 15, 15 }), scene.screenOf({ 55, 15 }));
		CHECK_EQUAL(255, scene.document.selection().coverage(55, 15));
		scene.root.commands.run(Command::Undo);
		CHECK_EQUAL(255, scene.document.selection().coverage(15, 15));
		CHECK_CLOSE(1.0f, scene.alphaAt(15, 15), 1e-6f);
	}

	TEST(TabletToolsMenuHasMove) {
		Document document;
		Brush brush;
		MoveViewport viewport;
		MoveIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		studio.useTool("move");
		CHECK(classic.canvas.tool() == CanvasTool::Move);
	}
}
