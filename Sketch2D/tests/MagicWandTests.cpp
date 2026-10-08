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
	class WandViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class WandIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	void paintRect(Document& document, int4 left, int4 top, int4 width, int4 height, uint1 red) {
		Layer& layer = *document.activeLayer();
		for (int4 y = top; y < top + height; y++)
			for (int4 x = left; x < left + width; x++) {
				const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
				Tile* tile = layer.tileAt(coord);
				uint1* pixel =
					tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
				pixel[0] = red;
				pixel[1] = pixel[2] = 0;
				pixel[3] = 255;
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
			}
	}

	struct WandScene {
		Document document;
		Brush brush;
		WandViewport viewport;
		WandIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		WandScene() {
			root.canvas.settings().viewStiffness = 1;
			paintRect(document, 0, 0, 100, 100, 255);
			paintRect(document, 100, 0, 100, 100, 100);
			document.commit("Shapes");
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
	};
}

SUITE(MagicWand) {
	TEST(ReferenceDefaults) {
		const SelectionSettings settings;
		CHECK_CLOSE(0.1f, settings.wand.threshold, 1e-6f);
		CHECK(settings.wand.allLayers);
		CHECK(!settings.wand.onlyAlpha);
	}

	TEST(SelectsTheMatchingRegion) {
		WandScene scene;
		CHECK(
			scene.document.magicWand({ 50, 50 }, BucketSettings{ 0.1f, false, true, 0 }, SelectionMode::Replace) ==
			BucketResult::Filled
		);
		CHECK_EQUAL(255, scene.document.selection().coverage(50, 50));
		CHECK_EQUAL(255, scene.document.selection().coverage(99, 99));
		CHECK_EQUAL(0, scene.document.selection().coverage(150, 50));
		CHECK_EQUAL(0, scene.document.selection().coverage(50, 150));
	}

	TEST(OpenAreasCanBeSelectedToo) {
		WandScene scene;
		CHECK(
			scene.document.magicWand({ 50, 150 }, BucketSettings{ 0.1f, false, true, 0 }, SelectionMode::Replace) ==
			BucketResult::Filled
		);
		CHECK_EQUAL(255, scene.document.selection().coverage(50, 150));
		CHECK_EQUAL(0, scene.document.selection().coverage(50, 50));
	}

	TEST(ThresholdJoinsSimilarColors) {
		WandScene scene;
		scene.document.magicWand({ 50, 50 }, BucketSettings{ 0.7f, false, true, 0 }, SelectionMode::Replace);
		CHECK_EQUAL(255, scene.document.selection().coverage(150, 50));
	}

	TEST(ToolClicksAddAndSubtract) {
		WandScene scene;
		scene.root.commands.run(Command::ToolMagicWand);
		CHECK(scene.root.canvas.tool() == CanvasTool::Wand);
		CHECK(scene.root.toolStrip.button("wand").isSelected());
		const size_t revision = scene.document.history().revision();
		scene.driver.click(scene.screenOf({ 50, 50 }));
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK_EQUAL(255, scene.document.selection().coverage(50, 50));
		scene.driver.click(scene.screenOf({ 150, 50 }), Modifiers{ true, false, false });
		CHECK_EQUAL(255, scene.document.selection().coverage(50, 50));
		CHECK_EQUAL(255, scene.document.selection().coverage(150, 50));
		scene.driver.click(scene.screenOf({ 50, 50 }), Modifiers{ false, false, true });
		CHECK_EQUAL(0, scene.document.selection().coverage(50, 50));
		CHECK_EQUAL(255, scene.document.selection().coverage(150, 50));
		scene.root.commands.run(Command::Undo);
		CHECK_EQUAL(255, scene.document.selection().coverage(50, 50));
	}

	TEST(EmptyCanvasExplains) {
		Document document;
		Brush brush;
		WandViewport viewport;
		WandIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };
		root.commands.run(Command::ToolMagicWand);
		driver.click(root.canvas.windowRect().pos + Vec2F{ 100, 100 });
		CHECK_EQUAL(std::string("You can't magic wand an empty area"), root.lastNotice());
	}

	TEST(MenuOptionsChangeTheWand) {
		WandScene scene;
		const auto entry = [&scene](const std::string& text) -> MenuItem* {
			for (const auto& item : scene.root.wandMenu.items())
				if (item->entry().text == text) return item.get();
			return nullptr;
		};
		REQUIRE CHECK(entry("Threshold 50%") != nullptr);
		CHECK(entry("Threshold 10%")->entry().isChecked());
		entry("Threshold 50%")->entry().action();
		CHECK_CLOSE(0.5f, scene.root.canvas.settings().selection.wand.threshold, 1e-6f);
		entry("Sample only alpha channel")->entry().action();
		CHECK(scene.root.canvas.settings().selection.wand.onlyAlpha);
		scene.root.commands.run(Command::ToolMagicWand);
		scene.driver.click(scene.screenOf({ 50, 50 }));
		CHECK_EQUAL(255, scene.document.selection().coverage(150, 50));
	}

	TEST(TabletToolsMenuHasTheWand) {
		Document document;
		Brush brush;
		WandViewport viewport;
		WandIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		studio.useTool("wand");
		CHECK(classic.canvas.tool() == CanvasTool::Wand);
	}
}
