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
	class CropViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class CropIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	void fillRect(Document& document, int4 left, int4 top, int4 width, int4 height) {
		Layer& layer = *document.activeLayer();
		for (int4 y = top; y < top + height; y++)
			for (int4 x = left; x < left + width; x++) {
				const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
				Tile* tile = layer.tileAt(coord);
				uint1* pixel =
					tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
				pixel[0] = 255;
				pixel[1] = pixel[2] = 0;
				pixel[3] = 255;
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
			}
	}

	struct CropScene {
		Document document;
		Brush brush;
		CropViewport viewport;
		CropIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		CropScene() {
			root.canvas.settings().viewStiffness = 1;
			fillRect(document, -300, -20, 700, 60);
			document.commit("Bar");
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		halnf alphaAt(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false).a; }
	};
}

SUITE(CropTool) {
	TEST(CropKeepsOnlyTheRegionAndFramesTheCanvas) {
		CropScene scene;
		CHECK(scene.document.cropTo(-50, -10, 150, 30) == CropResult::Cropped);
		CHECK(!scene.document.isInfinite());
		const CanvasFrame& frame = scene.document.frame();
		CHECK_EQUAL(-50, frame.left);
		CHECK_EQUAL(-10, frame.top);
		CHECK_EQUAL(150, frame.right);
		CHECK_EQUAL(30, frame.bottom);
		CHECK_CLOSE(1.0f, scene.alphaAt(-50, -10), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(149, 29), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(-51, 0), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(150, 0), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(0, 30), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(390, 0), 1e-6f);
		CHECK(scene.document.activeLayer()->tileIfExists({ 400 / kTileSize, 0 }) == nullptr);
	}

	TEST(CropAppliesToEveryLayer) {
		CropScene scene;
		Layer& bottom = *scene.document.activeLayer();
		scene.document.addLayer("Top");
		fillRect(scene.document, 200, 0, 10, 10);
		scene.document.cropTo(0, 0, 100, 100);
		CHECK_CLOSE(0.0f, scene.alphaAt(205, 5), 1e-6f);
		scene.document.setActiveLayer(&bottom);
		CHECK_CLOSE(0.0f, scene.alphaAt(300, 0), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(50, 10), 1e-6f);
	}

	TEST(AlphaLockedLayersRefuse) {
		CropScene scene;
		scene.document.toggleAlphaLock();
		CHECK(scene.document.cropTo(0, 0, 100, 100) == CropResult::AlphaLocked);
		CHECK(scene.document.isInfinite());
		CHECK_CLOSE(1.0f, scene.alphaAt(300, 0), 1e-6f);
	}

	TEST(EmptyRegionIsRefused) {
		CropScene scene;
		CHECK(scene.document.cropTo(10, 10, 10, 50) == CropResult::NoRegion);
		CHECK(scene.document.isInfinite());
	}

	TEST(ToolDragsARegionAndUndoes) {
		CropScene scene;
		scene.root.commands.run(Command::ToolPaint);
		scene.driver.click(scene.root.toolStrip.button("crop"));
		CHECK(scene.root.canvas.tool() == CanvasTool::Crop);
		const size_t revision = scene.document.history().revision();
		scene.driver.moveTo(scene.screenOf({ 20, 5 }));
		scene.driver.press();
		scene.driver.moveTo(scene.screenOf({ 100, 30 }));
		CHECK(scene.root.canvas.isCropping());
		size_t shades = 0;
		for (const PaintCommand& command : scene.driver.paint().commands())
			if (command.kind == PaintCommand::Kind::FillRect && command.color.a > 0.4f && command.color.a < 0.5f) shades++;
		CHECK_EQUAL((size_t) 4, shades);
		scene.driver.release();
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK_CLOSE(0.0f, scene.alphaAt(150, 0), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(50, 10), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(50, 2), 1e-6f);
		scene.root.commands.run(Command::Undo);
		CHECK(scene.document.isInfinite());
		CHECK_CLOSE(1.0f, scene.alphaAt(150, 0), 1e-6f);
	}

	TEST(ClickWithoutRegionAsks) {
		CropScene scene;
		scene.root.commands.run(Command::ToolPaint);
		scene.driver.click(scene.root.toolStrip.button("crop"));
		scene.driver.click(scene.screenOf({ 10, 10 }));
		CHECK_EQUAL(std::string("Please choose a region to crop"), scene.root.lastNotice());
		CHECK(scene.document.isInfinite());
	}

	TEST(ATinySlipDoesNotCrop) {
		CropScene scene;
		scene.root.commands.run(Command::ToolPaint);
		scene.driver.click(scene.root.toolStrip.button("crop"));
		scene.driver.drag(scene.screenOf({ 10, 10 }), scene.screenOf({ 12, 12 }));
		CHECK(scene.document.isInfinite());
		CHECK_CLOSE(1.0f, scene.alphaAt(300, 0), 1e-6f);
		CHECK_EQUAL(std::string("Please choose a region to crop"), scene.root.lastNotice());
	}

	TEST(CropCommandCutsToTheFrame) {
		CropScene scene;
		scene.document.setInfinite(false);
		scene.document.setFrame(0, 0, 100, 40);
		scene.document.commit("Frame");
		CHECK_CLOSE(1.0f, scene.alphaAt(300, 0), 1e-6f);
		scene.root.commands.run(Command::Crop);
		CHECK_CLOSE(0.0f, scene.alphaAt(300, 0), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(50, 0), 1e-6f);
	}

	TEST(CropCommandOnAnInfiniteCanvasPicksTheTool) {
		CropScene scene;
		scene.root.commands.run(Command::Crop);
		CHECK(scene.root.canvas.tool() == CanvasTool::Crop);
		CHECK_EQUAL(std::string("Please choose a region to crop"), scene.root.lastNotice());
	}

	TEST(AlphaLockShowsTheReferenceMessage) {
		CropScene scene;
		scene.document.toggleAlphaLock();
		scene.root.commands.run(Command::ToolPaint);
		scene.driver.click(scene.root.toolStrip.button("crop"));
		scene.driver.drag(scene.screenOf({ 0, 0 }), scene.screenOf({ 100, 30 }));
		CHECK_EQUAL(std::string("You can't crop a alpha locked layer"), scene.root.lastNotice());
	}

	TEST(TabletToolsMenuHasCrop) {
		Document document;
		Brush brush;
		CropViewport viewport;
		CropIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		studio.useTool("crop");
		CHECK(classic.canvas.tool() == CanvasTool::Crop);
	}
}
