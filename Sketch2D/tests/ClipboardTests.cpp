#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class ClipViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class ClipIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct ClipScene {
		Document document;
		Brush brush;
		ClipViewport viewport;
		ClipIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		UiDriver driver{ root, { 1280, 800 } };

		ClipScene() {
			Layer& layer = *document.activeLayer();
			for (int4 y = 10; y < 30; y++)
				for (int4 x = 10; x < 50; x++) {
					uint1* pixel = layer.tileAt({ 0, 0 })->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
					pixel[0] = 255;
					pixel[3] = 255;
				}
			document.commit("Shape");
		}

		halnf alphaAt(const Layer& layer, int4 x, int4 y) {
			const Tile* tile = layer.tileIfExists({ floordiv(x, kTileSize), floordiv(y, kTileSize) });
			if (!tile) return 0;
			return tile->pixels()
							 [((size_t) (y - floordiv(y, kTileSize) * kTileSize) * kTileSize +
								 (size_t) (x - floordiv(x, kTileSize) * kTileSize)) *
									4 +
								3] /
						 255.0f;
		}
	};
}

SUITE(Clipboard) {
	TEST(CopyKeepsPositionAndRespectsTheSelection) {
		ClipScene scene;
		scene.document.editSelection().addRectangle(0, 0, 20, 100, SelectionMode::Replace);
		const Document::PixelClip clip = scene.document.copyPixels();
		CHECK_EQUAL(10, clip.left);
		CHECK_EQUAL(10, clip.top);
		CHECK_EQUAL(10, clip.width);
		CHECK_EQUAL(20, clip.height);
		scene.document.editSelection().addRectangle(25, 15, 35, 100, SelectionMode::Replace);
		const Document::PixelClip inner = scene.document.copyPixels();
		CHECK_EQUAL(25, inner.left);
		CHECK_EQUAL(15, inner.top);
		CHECK_EQUAL(10, inner.width);
		CHECK_EQUAL(15, inner.height);
	}

	TEST(CopyPasteCreatesALayerAndStartsATransform) {
		ClipScene scene;
		Layer* original = scene.document.activeLayer();
		scene.driver.key(InputID::C, Modifiers{ false, true, false });
		CHECK(!scene.root.clipboard.isEmpty());
		const size_t layers = scene.document.layers().size();
		scene.driver.key(InputID::V, Modifiers{ false, true, false });
		CHECK_EQUAL(layers + 1, scene.document.layers().size());
		Layer* pasted = scene.document.activeLayer();
		CHECK(pasted != original);
		CHECK_EQUAL(std::string("Pasted"), pasted->name());
		CHECK_CLOSE(1.0f, scene.alphaAt(*pasted, 20, 20), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(*original, 20, 20), 1e-6f);
		CHECK(scene.root.canvas.isTransforming());
	}

	TEST(CutRemovesTheSelectedPixels) {
		ClipScene scene;
		Layer* original = scene.document.activeLayer();
		scene.document.editSelection().addRectangle(0, 0, 30, 100, SelectionMode::Replace);
		scene.root.commands.run(Command::Cut);
		CHECK_CLOSE(0.0f, scene.alphaAt(*original, 20, 20), 1e-6f);
		CHECK_CLOSE(1.0f, scene.alphaAt(*original, 40, 20), 1e-6f);
		scene.root.commands.run(Command::Paste);
		CHECK_CLOSE(1.0f, scene.alphaAt(*scene.document.activeLayer(), 20, 20), 1e-6f);
		CHECK_CLOSE(0.0f, scene.alphaAt(*scene.document.activeLayer(), 40, 20), 1e-6f);
		CHECK(!scene.document.hasSelection());
	}

	TEST(SoftSelectionsCopyPartially) {
		ClipScene scene;
		scene.document.editSelection().addRectangle(0, 0, 30, 100, SelectionMode::Replace);
		scene.document.editSelection().blur(3);
		const uint1 coverage = scene.document.selection().coverage(30, 20);
		scene.root.commands.run(Command::CopyToNewLayer);
		CHECK_CLOSE(coverage / 255.0f, scene.alphaAt(*scene.document.activeLayer(), 30, 20), 0.01f);
	}

	TEST(CopyToNewLayerIsOneUndoStep) {
		ClipScene scene;
		const size_t layers = scene.document.layers().size();
		const size_t revision = scene.document.history().revision();
		scene.root.commands.run(Command::CopyToNewLayer);
		CHECK_EQUAL(layers + 1, scene.document.layers().size());
		CHECK_EQUAL(revision + 1, scene.document.history().revision());
		CHECK_EQUAL(std::string("Copy"), scene.document.activeLayer()->name());
		scene.root.commands.run(Command::Undo);
		CHECK_EQUAL(layers, scene.document.layers().size());
	}

	TEST(EmptyCasesExplain) {
		ClipScene scene;
		scene.root.commands.run(Command::Paste);
		CHECK_EQUAL(std::string("The clipboard is empty"), scene.root.lastNotice());
		scene.document.addLayer("Empty");
		scene.root.commands.run(Command::Copy);
		CHECK_EQUAL(std::string("Nothing to copy"), scene.root.lastNotice());
		scene.root.commands.run(Command::Cut);
		CHECK_EQUAL(std::string("Nothing to cut"), scene.root.lastNotice());
	}
}
