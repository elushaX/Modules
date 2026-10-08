#include "TestSketchesRoot.hpp"
#include "Session.hpp"
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
	class BucketViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class BucketIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	void drawRing(Document& document, int4 left, int4 top, int4 right, int4 bottom, const RGBA& color, int4 gap = -1) {
		Layer& layer = *document.activeLayer();
		const uint1 pixel[4] = { (uint1) (color.r * 255), (uint1) (color.g * 255), (uint1) (color.b * 255), 255 };
		const auto put = [&](int4 x, int4 y) {
			const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
			Tile* tile = layer.tileAt(coord);
			uint1* target =
				tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
			for (int4 channel = 0; channel < 4; channel++)
				target[channel] = pixel[channel];
			tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
		};
		for (int4 x = left; x < right; x++) {
			for (int4 thick = 0; thick < 3; thick++) {
				if (gap < 0 || x < gap || x > gap + 4) put(x, top + thick);
				put(x, bottom - 1 - thick);
			}
		}
		for (int4 y = top; y < bottom; y++)
			for (int4 thick = 0; thick < 3; thick++) {
				put(left + thick, y);
				put(right - 1 - thick, y);
			}
	}

	struct BucketScene {
		Document document;
		Brush brush;
		BucketViewport viewport;
		BucketIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		BucketScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
		}

		Vec2F screenOf(Vec2F world) { return root.canvas.windowRect().pos + document.worldToScreen(world); }
		RGBA at(int4 x, int4 y) { return document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false); }
	};

	void checkRgba(const RGBA& expected, const RGBA& actual, halnf tolerance = 0.01f) {
		CHECK_CLOSE(expected.r, actual.r, tolerance);
		CHECK_CLOSE(expected.g, actual.g, tolerance);
		CHECK_CLOSE(expected.b, actual.b, tolerance);
		CHECK_CLOSE(expected.a, actual.a, tolerance);
	}
}

SUITE(PaintBucket) {
	TEST(ReferenceDefaults) {
		const BucketSettings settings;
		CHECK_CLOSE(0.1f, settings.threshold, 1e-6f);
		CHECK(!settings.onlyAlpha);
		CHECK(settings.allLayers);
		CHECK_EQUAL(1, settings.spread);
	}

	TEST(FillsAnEnclosedAreaAndStopsAtTheLine) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		BucketSettings settings;
		settings.spread = 0;
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, settings) == BucketResult::Filled);
		checkRgba({ 1, 0, 0, 1 }, scene.at(30, 20));
		checkRgba({ 1, 0, 0, 1 }, scene.at(3, 3));
		checkRgba({ 1, 0, 0, 1 }, scene.at(56, 36));
		checkRgba({ 0, 0, 0, 1 }, scene.at(2, 20));
		CHECK_CLOSE(0.0f, scene.at(70, 20).a, 1e-6f);
	}

	TEST(SpreadGrowsTheFillUnderTheLine) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		BucketSettings settings;
		settings.spread = 1;
		scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, settings);
		checkRgba({ 1, 0, 0, 1 }, scene.at(2, 20));
		checkRgba({ 0, 0, 0, 1 }, scene.at(1, 20));
	}

	TEST(AGapInTheLineIsALeak) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 }, 20);
		const size_t revision = scene.document.history().revision();
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, BucketSettings()) == BucketResult::Leak);
		CHECK_CLOSE(0.0f, scene.at(30, 20).a, 1e-6f);
		CHECK_EQUAL(revision, scene.document.history().revision());
	}

	TEST(AFramedCanvasFillsUpToItsEdge) {
		BucketScene scene;
		scene.document.setInfinite(false);
		scene.document.setFrame(0, 0, 100, 80);
		drawRing(scene.document, 10, 10, 40, 40, { 0, 0, 0, 1 }, 20);
		CHECK(scene.document.bucketFill({ 70, 60 }, { 0, 0, 1, 1 }, BucketSettings()) == BucketResult::Filled);
		checkRgba({ 0, 0, 1, 1 }, scene.at(99, 79));
		checkRgba({ 0, 0, 1, 1 }, scene.at(25, 25));
		CHECK_CLOSE(0.0f, scene.at(100, 40).a, 1e-6f);
	}

	TEST(EmptyCanvasIsRefused) {
		BucketScene scene;
		CHECK(scene.document.bucketFill({ 10, 10 }, { 1, 0, 0, 1 }, BucketSettings()) == BucketResult::Empty);
	}

	TEST(ThresholdDecidesWhichShadesJoinTheFill) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		drawRing(scene.document, 20, 10, 40, 30, { 0.95f, 0.95f, 0.95f, 1 });
		BucketSettings settings;
		settings.spread = 0;
		settings.threshold = 0.0f;
		scene.document.bucketFill({ 10, 20 }, { 1, 0, 0, 1 }, settings);
		checkRgba({ 0.95f, 0.95f, 0.95f, 1 }, scene.at(21, 20));
		CHECK_CLOSE(0.0f, scene.at(30, 20).a, 1e-6f);

		BucketScene loose;
		drawRing(loose.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		Layer& layer = *loose.document.activeLayer();
		for (int4 y = 3; y < 37; y++)
			for (int4 x = 3; x < 57; x++) {
				uint1* pixel = layer.tileAt({ 0, 0 })->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
				const uint1 value = x < 30 ? 200 : 210;
				pixel[0] = pixel[1] = pixel[2] = value;
				pixel[3] = 255;
			}
		BucketSettings tight;
		tight.spread = 0;
		tight.threshold = 0.02f;
		loose.document.bucketFill({ 10, 20 }, { 1, 0, 0, 1 }, tight);
		checkRgba({ 1, 0, 0, 1 }, loose.at(10, 20));
		CHECK_CLOSE(210.0f / 255.0f, loose.at(40, 20).r, 0.01f);
		BucketSettings wide;
		wide.spread = 0;
		wide.threshold = 0.1f;
		loose.document.bucketFill({ 40, 20 }, { 0, 1, 0, 1 }, wide);
		checkRgba({ 0, 1, 0, 1 }, loose.at(40, 20));
	}

	TEST(SampleAllLayersUsesLinesOnOtherLayers) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		scene.document.addLayer("Color");
		BucketSettings settings;
		settings.spread = 0;
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, settings) == BucketResult::Filled);
		checkRgba({ 1, 0, 0, 1 }, scene.at(30, 20));
		CHECK_CLOSE(0.0f, scene.at(1, 20).a, 1e-6f);

		settings.allLayers = false;
		scene.document.addLayer("Other");
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, settings) == BucketResult::Leak);
	}

	TEST(HiddenLayersDoNotBlockTheFill) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		drawRing(scene.document, 20, 10, 40, 30, { 0, 0, 0, 1 });
		Layer& lines = *scene.document.activeLayer();
		scene.document.addLayer("Color");
		BucketSettings settings;
		settings.spread = 0;
		scene.document.setLayerVisible(lines, false);
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, settings) == BucketResult::Leak);
		scene.document.setLayerVisible(lines, true);
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, settings) == BucketResult::Filled);
		checkRgba({ 1, 0, 0, 1 }, scene.at(30, 20));
		CHECK_CLOSE(0.0f, scene.at(10, 20).a, 1e-6f);
	}

	TEST(OnlyAlphaIgnoresColorDifferences) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		Layer& layer = *scene.document.activeLayer();
		for (int4 y = 3; y < 37; y++)
			for (int4 x = 3; x < 57; x++) {
				uint1* pixel = layer.tileAt({ 0, 0 })->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
				const uint1 value = x < 30 ? 40 : 230;
				pixel[0] = pixel[1] = pixel[2] = value;
				pixel[3] = 255;
			}
		BucketSettings settings;
		settings.spread = 0;
		settings.threshold = 0.05f;
		CHECK(scene.document.bucketFill({ 10, 20 }, { 0, 0, 1, 1 }, settings) == BucketResult::Filled);
		checkRgba({ 0, 0, 1, 1 }, scene.at(10, 20));
		CHECK_CLOSE(230.0f / 255.0f, scene.at(40, 20).r, 0.01f);
		checkRgba({ 0, 0, 0, 1 }, scene.at(1, 20));

		settings.onlyAlpha = true;
		CHECK(scene.document.bucketFill({ 40, 20 }, { 1, 0, 0, 1 }, settings) == BucketResult::Filled);
		checkRgba({ 1, 0, 0, 1 }, scene.at(40, 20));
		checkRgba({ 1, 0, 0, 1 }, scene.at(10, 20));
		checkRgba({ 1, 0, 0, 1 }, scene.at(1, 20));
		CHECK_CLOSE(0.0f, scene.at(70, 20).a, 1e-6f);
	}

	TEST(LockedLayersAreNotFilled) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 60, 40, { 0, 0, 0, 1 });
		scene.document.toggleLock();
		CHECK(scene.document.bucketFill({ 30, 20 }, { 1, 0, 0, 1 }, BucketSettings()) == BucketResult::Locked);
	}

	TEST(ToolFillsOnClickAndUndoes) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 120, 100, { 0, 0, 0, 1 });
		scene.document.commit("Ring");
		scene.brush.color = { 0, 0.5f, 1, 1 };
		scene.root.commands.run(Command::ToolBucket);
		CHECK(scene.root.canvas.tool() == CanvasTool::Bucket);
		CHECK(scene.root.toolStrip.button("fill").isSelected());
		const size_t revision = scene.document.history().revision();
		scene.driver.click(scene.screenOf({ 60, 50 }));
		CHECK(scene.document.history().revision() > revision);
		checkRgba({ 0, 0.5f, 1, 1 }, scene.at(60, 50));
		scene.root.commands.run(Command::Undo);
		CHECK_CLOSE(0.0f, scene.at(60, 50).a, 1e-6f);
	}

	TEST(ToolUsesBrushOpacity) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 120, 100, { 0, 0, 0, 1 });
		scene.brush.color = { 1, 0, 0, 1 };
		scene.brush.setOpacity(0.5f);
		scene.root.commands.run(Command::ToolBucket);
		scene.driver.click(scene.screenOf({ 60, 50 }));
		CHECK_CLOSE(0.5f, scene.at(60, 50).a, 0.01f);
	}

	TEST(LeakShowsTheReferenceMessage) {
		BucketScene scene;
		drawRing(scene.document, 0, 0, 120, 100, { 0, 0, 0, 1 }, 40);
		scene.root.commands.run(Command::ToolBucket);
		scene.driver.click(scene.screenOf({ 60, 50 }));
		CHECK_EQUAL(
			std::string("Found a leak. Please choose an enclosed area or lower threshold"), scene.root.lastNotice()
		);
	}

	TEST(BucketSettingsMenuAndSession) {
		BucketScene scene;
		BucketSettings& settings = scene.root.canvas.settings().bucket;
		const auto entry = [&scene](const std::string& text) -> MenuItem* {
			for (const auto& item : scene.root.bucketMenu.items())
				if (item->entry().text == text) return item.get();
			return nullptr;
		};
		REQUIRE CHECK(entry("Sample all visible layers") != nullptr);
		REQUIRE CHECK(entry("Sample only alpha channel") != nullptr);
		REQUIRE CHECK(entry("Threshold 35%") != nullptr);
		REQUIRE CHECK(entry("Expand 4 px") != nullptr);
		CHECK(entry("Threshold 10%")->entry().isChecked());
		entry("Threshold 35%")->entry().action();
		entry("Expand 4 px")->entry().action();
		entry("Sample only alpha channel")->entry().action();
		entry("Sample all visible layers")->entry().action();
		CHECK_CLOSE(0.35f, settings.threshold, 1e-6f);
		CHECK_EQUAL(4, settings.spread);
		CHECK(settings.onlyAlpha);
		CHECK(!settings.allLayers);

		const auto restored = SessionSnapshot::parse(
			SessionSnapshot::capture(scene.document, scene.brush, scene.root.canvas.settings(), {}).serialize()
		);
		REQUIRE CHECK(restored.has_value());
		CHECK_CLOSE(0.35f, restored->settings.bucket.threshold, 1e-4f);
		CHECK_EQUAL(4, restored->settings.bucket.spread);
		CHECK(restored->settings.bucket.onlyAlpha);
		CHECK(!restored->settings.bucket.allLayers);
	}

	TEST(TabletToolsMenuHasTheBucket) {
		Document document;
		Brush brush;
		BucketViewport viewport;
		BucketIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		CHECK(studio.groupOf("fill") != nullptr);
		studio.useTool("fill");
		CHECK(classic.canvas.tool() == CanvasTool::Bucket);
	}
}
