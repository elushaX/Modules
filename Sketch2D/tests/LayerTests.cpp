#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class LayerViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class LayerIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct LayerScene {
		Document document;
		Brush brush;
		LayerViewport viewport;
		LayerIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };

		bool run(Command command) { return root.commands.run(command); }
	};

	Brush solid() {
		Brush brush;
		brush.color = { 1, 0, 0, 1 };
		brush.setFlow(1);
		brush.setHardness(1);
		brush.setSize(20);
		return brush;
	}

	const uint1* pixelAt(const Document& document, int4 x, int4 y) {
		document.settlePaint();
		const Tile* tile = document.activeLayer()->tileIfExists({ x / kTileSize, y / kTileSize });
		return tile ? tile->pixels() + ((ualni) (y % kTileSize) * kTileSize + (x % kTileSize)) * 4 : nullptr;
	}
}

SUITE(LayerProperties) {
	TEST(BlendModesFollowReferenceOrder) {
		CHECK_EQUAL("Normal", blendModeName(BlendMode::Normal));
		CHECK_EQUAL("Soft dodge", blendModeName(BlendMode::SoftDodge));
		CHECK(nextBlendMode(BlendMode::Color) == BlendMode::Normal);
		CHECK(previousBlendMode(BlendMode::Normal) == BlendMode::Color);
		CHECK(nextBlendMode(BlendMode::Darken) == BlendMode::ColorDodge);
	}

	TEST(BlendCommandsCycleAndMergeIntoOneStep) {
		LayerScene scene;
		scene.run(Command::NextLayerBlend);
		scene.run(Command::NextLayerBlend);
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Screen);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
		CHECK_EQUAL("Layer blend mode", scene.document.history().undoLabel());

		scene.run(Command::PreviousLayerBlend);
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Multiply);
		scene.run(Command::NormalLayerBlend);
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Normal);

		scene.document.undo();
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Normal);
	}

	TEST(OpacityPresetsAndSteps) {
		LayerScene scene;
		scene.run(Command::LayerOpacity40);
		CHECK_CLOSE(0.4f, scene.document.activeLayer()->opacity(), 0.001f);
		CHECK(scene.root.commands.isChecked(Command::LayerOpacity40));

		scene.run(Command::LayerOpacityIncrease);
		CHECK_CLOSE(0.5f, scene.document.activeLayer()->opacity(), 0.001f);

		scene.run(Command::LayerOpacity100);
		scene.run(Command::LayerOpacityIncrease);
		CHECK_CLOSE(1.0f, scene.document.activeLayer()->opacity(), 0.001f);

		scene.run(Command::LayerOpacity0);
		scene.run(Command::LayerOpacityDecrease);
		CHECK_CLOSE(0.0f, scene.document.activeLayer()->opacity(), 0.001f);
	}

	TEST(QuickRepeatOfPresetSelectsFineOpacity) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::LayerOpacity40);
		driver.advance(100);
		scene.run(Command::LayerOpacity40);
		CHECK_CLOSE(0.04f, scene.document.activeLayer()->opacity(), 0.001f);

		driver.advance(400);
		scene.run(Command::LayerOpacity40);
		CHECK_CLOSE(0.4f, scene.document.activeLayer()->opacity(), 0.001f);
	}

	TEST(ClipNeedsALayerBelow) {
		LayerScene scene;
		CHECK(!scene.root.commands.isEnabled(Command::ToggleLayerClip));

		scene.run(Command::NewLayer);
		CHECK(scene.root.commands.isEnabled(Command::ToggleLayerClip));
		scene.run(Command::ToggleLayerClip);
		CHECK(scene.document.activeLayer()->clipped());
		CHECK(scene.root.commands.isChecked(Command::ToggleLayerClip));
		CHECK_EQUAL("Lock/unlock layer", scene.document.history().undoLabel());
	}

	TEST(LockedLayerRejectsPaintAndProperties) {
		LayerScene scene;
		scene.run(Command::ToggleLayerLock);
		CHECK(scene.document.activeLayer()->locked());

		scene.document.stampBrushDab({ 50, 50 }, solid());
		CHECK_EQUAL(0u, scene.document.tileCount());

		scene.run(Command::NextLayerBlend);
		scene.run(Command::LayerOpacity50);
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Normal);
		CHECK_CLOSE(1.0f, scene.document.activeLayer()->opacity(), 0.001f);

		scene.run(Command::ToggleLayerLock);
		scene.document.stampBrushDab({ 50, 50 }, solid());
		CHECK(scene.document.tileCount() > 0);
	}

	TEST(AlphaLockKeepsCoverageAndBlocksErasing) {
		LayerScene scene;
		Brush red = solid();
		red.color = { 1, 0, 0, 0.5f };
		red.setFlow(0.5f);
		scene.document.stampBrushDab({ 50, 50 }, red);
		scene.document.finishStroke();
		const uint1 alpha = pixelAt(scene.document, 50, 50)[3];

		scene.run(Command::ToggleLayerAlphaLock);
		Brush blue = solid();
		blue.color = { 0, 0, 1, 1 };
		scene.document.stampBrushDab({ 50, 50 }, blue);
		scene.document.stampBrushDab({ 400, 50 }, blue);

		const uint1* pixel = pixelAt(scene.document, 50, 50);
		CHECK_EQUAL((int) alpha, (int) pixel[3]);
		CHECK((int) pixel[2] > 100);
		CHECK((int) pixel[2] <= (int) pixel[3]);
		CHECK(pixelAt(scene.document, 400, 50) == nullptr);

		Brush eraser = solid();
		eraser.type = BrushType::Eraser;
		scene.document.stampBrushDab({ 50, 50 }, eraser);
		CHECK_EQUAL((int) alpha, (int) pixelAt(scene.document, 50, 50)[3]);
	}

	TEST(VisibilityCommandIsCheckedAndUndoable) {
		LayerScene scene;
		CHECK(scene.root.commands.isChecked(Command::ToggleLayerVisible));
		scene.run(Command::ToggleLayerVisible);
		CHECK(!scene.document.activeLayer()->visible());
		CHECK_EQUAL("Show/hide layer", scene.document.history().undoLabel());
		scene.document.undo();
		CHECK(scene.document.activeLayer()->visible());
	}

	TEST(SelectedLayersChangeTogether) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		bottom->setSelected(true);

		scene.run(Command::LayerOpacity30);
		CHECK_CLOSE(0.3f, bottom->opacity(), 0.001f);
		CHECK_CLOSE(0.3f, scene.document.activeLayer()->opacity(), 0.001f);
	}

	TEST(LayerMenuShowsStateAndSubmenus) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NextLayerBlend);

		scene.root.layerBlendMenu.refresh();
		CHECK(scene.root.layerBlendMenu.items()[1]->entry().isChecked());
		CHECK(!scene.root.layerBlendMenu.items()[0]->entry().isChecked());

		scene.root.layerBlendMenu.items()[5]->activate();
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Darken);
	}
}

SUITE(LayerStructure) {
	TEST(NewLayersGoAboveActiveWithUniqueNames) {
		LayerScene scene;
		scene.run(Command::NewLayer);
		scene.run(Command::NewLayer);
		CHECK_EQUAL("Layer 3", scene.document.activeLayer()->name());

		scene.document.setActiveLayer(scene.document.layers()[0].get());
		scene.run(Command::NewLayer);
		CHECK_EQUAL(1u, scene.document.indexOf(*scene.document.activeLayer()));
		CHECK_EQUAL("Layer 4", scene.document.activeLayer()->name());
		CHECK_EQUAL("Create layer", scene.document.history().undoLabel());
	}

	TEST(NewLayerAboveClippedLayerIsClipped) {
		LayerScene scene;
		scene.run(Command::NewLayer);
		scene.run(Command::ToggleLayerClip);
		scene.run(Command::NewLayer);
		CHECK(scene.document.activeLayer()->clipped());
	}

	TEST(DuplicateCopiesPixelsAndPropertiesBesideSource) {
		LayerScene scene;
		scene.document.stampBrushDab({ 50, 50 }, solid());
		scene.document.finishStroke();
		scene.run(Command::NextLayerBlend);
		scene.run(Command::LayerOpacity60);
		scene.run(Command::ToggleLayerAlphaLock);

		scene.run(Command::DuplicateLayer);
		CHECK_EQUAL(2u, scene.document.layerCount());
		Layer& copy = *scene.document.activeLayer();
		CHECK_EQUAL("Layer 2", copy.name());
		CHECK(copy.blend() == BlendMode::Multiply);
		CHECK_CLOSE(0.6f, copy.opacity(), 0.001f);
		CHECK(copy.alphaLocked());
		CHECK_EQUAL(255, (int) pixelAt(scene.document, 50, 50)[3]);
		CHECK_EQUAL("Duplicate layer", scene.document.history().undoLabel());
	}

	TEST(DuplicatedPixelsAreIndependent) {
		LayerScene scene;
		scene.document.stampBrushDab({ 50, 50 }, solid());
		scene.document.finishStroke();
		Layer* original = scene.document.activeLayer();
		scene.run(Command::DuplicateLayer);

		Brush eraser = solid();
		eraser.type = BrushType::Eraser;
		eraser.setSize(60);
		scene.document.stampBrushDab({ 50, 50 }, eraser);
		scene.document.finishStroke("Eraser");

		const Tile* tile = original->tileIfExists({ 0, 0 });
		CHECK(tile != nullptr);
		CHECK_EQUAL(255, (int) tile->pixels()[(50 * kTileSize + 50) * 4 + 3]);
	}

	TEST(DuplicateOfNumberedLayerTakesNextNumber) {
		LayerScene scene;
		scene.document.renameLayer(*scene.document.activeLayer(), "Sketch");
		scene.run(Command::DuplicateLayer);
		CHECK_EQUAL("Sketch 1", scene.document.activeLayer()->name());
		scene.run(Command::DuplicateLayer);
		CHECK_EQUAL("Sketch 2", scene.document.activeLayer()->name());
	}

	TEST(DeleteSelectsLayerBelowAndRefusesLocked) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		scene.run(Command::ToggleLayerLock);

		scene.run(Command::DeleteLayer);
		CHECK_EQUAL(2u, scene.document.layerCount());
		CHECK_EQUAL("You can't delete a locked layer", scene.root.lastNotice());

		scene.run(Command::ToggleLayerLock);
		scene.run(Command::DeleteLayer);
		CHECK_EQUAL(1u, scene.document.layerCount());
		CHECK(scene.document.activeLayer() == bottom);
		CHECK(!scene.root.commands.isEnabled(Command::DeleteLayer));
	}

	TEST(MoveUpAndDownReorderAndRespectEnds) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		Layer* top = scene.document.activeLayer();
		CHECK(!scene.root.commands.isEnabled(Command::MoveLayerUp));

		scene.run(Command::MoveLayerDown);
		CHECK(scene.document.layers()[0].get() == top);
		CHECK(scene.document.layers()[1].get() == bottom);
		CHECK(!scene.root.commands.isEnabled(Command::MoveLayerDown));
		CHECK_EQUAL("Move layer up/down", scene.document.history().undoLabel());

		scene.run(Command::MoveLayerUp);
		CHECK(scene.document.layers()[1].get() == top);

		scene.document.undo();
		CHECK(scene.document.layers()[0].get() == top);
	}

	TEST(MovingClippedLayerToBottomUnclipsIt) {
		LayerScene scene;
		scene.run(Command::NewLayer);
		scene.run(Command::ToggleLayerClip);
		scene.run(Command::MoveLayerDown);
		CHECK(!scene.document.activeLayer()->clipped());
	}

	TEST(RenameDialogRenamesAndSanitizes) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.run(Command::RenameLayer);
		CHECK(scene.root.renameLayer.isOpen());
		CHECK_EQUAL("Layer 1", scene.root.renameLayer.name.text());
		CHECK(scene.root.renameLayer.name.isEditing());

		driver.type("Ink \"line\"");
		driver.key(InputID::ENTER);
		CHECK(!scene.root.renameLayer.isOpen());
		CHECK_EQUAL("Ink  line ", scene.document.activeLayer()->name());
		CHECK_EQUAL("Rename layer", scene.document.history().undoLabel());

		scene.document.undo();
		CHECK_EQUAL("Layer 1", scene.document.activeLayer()->name());
	}

	TEST(RenameCancelAndUnchangedNameDoNotCommit) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.run(Command::RenameLayer);
		driver.type("Other");
		driver.key(InputID::ESCAPE);
		CHECK_EQUAL("Layer 1", scene.document.activeLayer()->name());

		scene.run(Command::RenameLayer);
		driver.click(*scene.root.renameLayer.buttons()[0]);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(LayerNamesAreLimitedTo63Characters) {
		LayerScene scene;
		scene.document.renameLayer(*scene.document.activeLayer(), std::string(100, 'a'));
		CHECK_EQUAL(63u, scene.document.activeLayer()->name().size());
	}

	TEST(LayerCountIsCappedAt256) {
		LayerScene scene;
		for (int layer = 1; layer < 256; layer++)
			scene.document.addLayer(scene.document.nextLayerName());
		CHECK_EQUAL(256u, scene.document.layerCount());
		CHECK(!scene.root.commands.isEnabled(Command::NewLayer));
		CHECK(!scene.root.commands.isEnabled(Command::DuplicateLayer));
		CHECK(scene.document.addLayer("extra") == nullptr);
	}
}

SUITE(LayerSelection) {
	TEST(SelectAboveAndBelowMoveActiveAndClearSelection) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		Layer* top = scene.document.activeLayer();
		scene.run(Command::SelectAllLayers);
		CHECK_EQUAL(2u, scene.document.selectedCount());

		scene.run(Command::SelectLayerBelow);
		CHECK(scene.document.activeLayer() == bottom);
		CHECK_EQUAL(1u, scene.document.selectedCount());
		CHECK_EQUAL("Select layer", scene.document.history().undoLabel());

		scene.run(Command::SelectLayerBelow);
		CHECK(scene.document.activeLayer() == bottom);
		scene.run(Command::SelectLayerAbove);
		CHECK(scene.document.activeLayer() == top);
	}

	TEST(SelectionStepsMergeIntoOneHistoryEntry) {
		LayerScene scene;
		scene.run(Command::NewLayer);
		scene.run(Command::NewLayer);
		const size_t before = scene.document.history().entries().size();

		scene.run(Command::SelectLayerBelow);
		scene.run(Command::SelectLayerBelow);
		CHECK_EQUAL(before + 1, scene.document.history().entries().size());
	}

	TEST(SelectAllTargetsEveryLayerForPropertyChanges) {
		LayerScene scene;
		scene.run(Command::NewLayer);
		scene.run(Command::NewLayer);
		scene.run(Command::SelectAllLayers);

		scene.run(Command::LayerOpacity20);
		for (const auto& layer : scene.document.layers())
			CHECK_CLOSE(0.2f, layer->opacity(), 0.001f);

		scene.run(Command::ToggleLayerVisible);
		for (const auto& layer : scene.document.layers())
			CHECK(!layer->visible());
	}

	TEST(SelectAllShortcutIsCtrlAltA) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NewLayer);
		Modifiers modifiers;
		modifiers.control = true;
		modifiers.alt = true;
		driver.key(InputID::A, modifiers);
		CHECK_EQUAL(2u, scene.document.selectedCount());
	}

	TEST(CtrlClickTogglesAndShiftClickSelectsRange) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NewLayer);
		scene.run(Command::NewLayer);
		scene.run(Command::NewLayer);
		LayersPopup& popup = scene.root.toolBar.layersPopup;
		driver.click(scene.root.toolBar.layers);

		Modifiers control;
		control.control = true;
		driver.click(popup.rows()[3]->name, control);
		CHECK_EQUAL(2u, scene.document.selectedCount());
		CHECK(popup.rows()[3]->name.isSelected());
		CHECK(&popup.rows()[3]->layer() == scene.document.layers()[0].get());

		driver.advance(600);
		driver.click(popup.rows()[3]->name, control);
		CHECK_EQUAL(1u, scene.document.selectedCount());

		Modifiers shift;
		shift.shift = true;
		driver.advance(600);
		driver.click(popup.rows()[2]->name);
		driver.click(popup.rows()[0]->name, shift);
		CHECK_EQUAL(3u, scene.document.selectedCount());
		CHECK(!scene.document.layers()[0]->selected());

		driver.advance(600);
		driver.click(popup.rows()[1]->name);
		CHECK_EQUAL(1u, scene.document.selectedCount());
		CHECK(scene.document.activeLayer() == scene.document.layers()[2].get());
	}

	TEST(CtrlClickOnActiveMovesActiveToRemainingSelection) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		Layer* top = scene.document.activeLayer();
		scene.document.toggleLayerSelection(*bottom);

		scene.document.toggleLayerSelection(*top);
		CHECK(scene.document.activeLayer() == bottom);
		CHECK_EQUAL(1u, scene.document.selectedCount());
		CHECK(!scene.document.toggleLayerSelection(*bottom));
	}

	TEST(DoubleClickOnRowOpensRename) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.click(scene.root.toolBar.layers);
		LayersPopup& popup = scene.root.toolBar.layersPopup;

		driver.click(popup.rows()[0]->name);
		driver.click(popup.rows()[0]->name);
		CHECK(scene.root.renameLayer.isOpen());
	}

	TEST(SelectionIsUndoable) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		scene.run(Command::SelectAllLayers);
		scene.document.undo();
		CHECK(!bottom->selected());
		CHECK_EQUAL(1u, scene.document.selectedCount());
	}
}

#include "Compositor.hpp"

namespace {
	void fillLayer(Layer& layer, const TileCoord& coord, uint1 r, uint1 g, uint1 b, uint1 a) {
		uint1* pixels = layer.tileAt(coord)->pixels();
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize; index++) {
			pixels[index * 4 + 0] = r;
			pixels[index * 4 + 1] = g;
			pixels[index * 4 + 2] = b;
			pixels[index * 4 + 3] = a;
		}
	}

	std::vector<int> visiblePixel(const Document& document, const TileCoord& coord) {
		Compositor compositor;
		compositor.update(document);
		const Tile* tile = compositor.tileAt(coord);
		if (!tile) return { 0, 0, 0, 0 };
		return { tile->pixels()[0], tile->pixels()[1], tile->pixels()[2], tile->pixels()[3] };
	}

	void checkSameColor(const std::vector<int>& expected, const std::vector<int>& actual) {
		for (size_t channel = 0; channel < 4; channel++)
			CHECK_CLOSE(expected[channel], actual[channel], 1);
	}
}

SUITE(LayerMerging) {
	TEST(MergeDownKeepsLookAndLowerLayer) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		scene.document.renameLayer(*bottom, "Base");
		fillLayer(*bottom, { 0, 0 }, 40, 80, 160, 255);
		scene.run(Command::NewLayer);
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 200, 100, 0, 200);
		fillLayer(*scene.document.activeLayer(), { 1, 0 }, 200, 100, 0, 200);
		scene.run(Command::NextLayerBlend);
		scene.run(Command::LayerOpacity60);
		const std::vector<int> before = visiblePixel(scene.document, { 0, 0 });
		const std::vector<int> beforeOutside = visiblePixel(scene.document, { 1, 0 });

		scene.run(Command::MergeLayers);
		CHECK_EQUAL(1u, scene.document.layerCount());
		CHECK(scene.document.activeLayer() == bottom);
		CHECK_EQUAL("Base", bottom->name());
		CHECK(bottom->blend() == BlendMode::Normal);
		checkSameColor(before, visiblePixel(scene.document, { 0, 0 }));
		checkSameColor(beforeOutside, visiblePixel(scene.document, { 1, 0 }));
		CHECK_EQUAL("Merge layers", scene.document.history().undoLabel());

		scene.document.undo();
		CHECK_EQUAL(2u, scene.document.layerCount());
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Multiply);
	}

	TEST(MergeBakesLowerLayerOpacity) {
		LayerScene scene;
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 255, 255, 255, 255);
		scene.run(Command::LayerOpacity50);
		scene.run(Command::NewLayer);
		const std::vector<int> before = visiblePixel(scene.document, { 0, 0 });

		scene.run(Command::MergeLayers);
		CHECK_CLOSE(1.0f, scene.document.activeLayer()->opacity(), 0.001f);
		checkSameColor(before, visiblePixel(scene.document, { 0, 0 }));
	}

	TEST(MergeSelectedFlattensIntoLowestSelected) {
		LayerScene scene;
		Layer* bottom = scene.document.activeLayer();
		fillLayer(*bottom, { 0, 0 }, 255, 0, 0, 255);
		scene.run(Command::NewLayer);
		Layer* middle = scene.document.activeLayer();
		fillLayer(*middle, { 0, 0 }, 0, 255, 0, 128);
		scene.run(Command::NewLayer);
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 0, 0, 255, 128);
		scene.document.toggleLayerSelection(*middle);
		const std::vector<int> before = visiblePixel(scene.document, { 0, 0 });

		scene.run(Command::MergeLayers);
		CHECK_EQUAL(2u, scene.document.layerCount());
		CHECK(scene.document.activeLayer() == middle);
		CHECK(scene.document.layers()[0].get() == bottom);
		checkSameColor(before, visiblePixel(scene.document, { 0, 0 }));
	}

	TEST(MergeIsDisabledForBottomLayerAlone) {
		LayerScene scene;
		scene.run(Command::NewLayer);
		scene.run(Command::SelectLayerBelow);
		CHECK(!scene.root.commands.isEnabled(Command::MergeLayers));
		scene.run(Command::SelectLayerAbove);
		CHECK(scene.root.commands.isEnabled(Command::MergeLayers));
	}

	TEST(MergedClippedLayerStaysInsideBase) {
		LayerScene scene;
		Layer* base = scene.document.activeLayer();
		fillLayer(*base, { 0, 0 }, 255, 255, 255, 255);
		scene.run(Command::NewLayer);
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fillLayer(*scene.document.activeLayer(), { 1, 0 }, 255, 0, 0, 255);
		scene.run(Command::ToggleLayerClip);

		scene.run(Command::MergeLayers);
		CHECK(base->tileIfExists({ 1, 0 }) == nullptr);
		checkSameColor({ 255, 0, 0, 255 }, visiblePixel(scene.document, { 0, 0 }));
	}

	TEST(MergeAllAsksThenMergesVisibleLayers) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		scene.run(Command::NewLayer);
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 0, 0, 255, 100);
		scene.run(Command::NewLayer);
		Layer* hidden = scene.document.activeLayer();
		scene.run(Command::ToggleLayerVisible);

		scene.run(Command::MergeAllLayers);
		CHECK(scene.root.confirm.isOpen());
		CHECK_EQUAL("Are you sure you would like to merge all layers?", scene.root.confirm.message());
		CHECK_EQUAL("Merge all", scene.root.confirm.buttons()[0]->text());
		CHECK_EQUAL(3u, scene.document.layerCount());

		driver.click(*scene.root.confirm.buttons()[0]);
		CHECK_EQUAL(2u, scene.document.layerCount());
		CHECK(scene.document.layers()[1].get() == hidden);
		CHECK_EQUAL("Merge all layers", scene.document.history().undoLabel());
	}

	TEST(BakeOpacityKeepsLookAndResetsOpacity) {
		LayerScene scene;
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 200, 100, 50, 255);
		scene.run(Command::LayerOpacity40);
		const std::vector<int> before = visiblePixel(scene.document, { 0, 0 });
		CHECK(scene.root.commands.isEnabled(Command::BakeLayerOpacity));

		scene.run(Command::BakeLayerOpacity);
		CHECK_CLOSE(1.0f, scene.document.activeLayer()->opacity(), 0.001f);
		checkSameColor(before, visiblePixel(scene.document, { 0, 0 }));
		CHECK_EQUAL("Bake layer opacity", scene.document.history().undoLabel());
		CHECK(!scene.root.commands.isEnabled(Command::BakeLayerOpacity));
	}

	TEST(ClearLayerRespectsLocks) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);

		scene.run(Command::ToggleLayerAlphaLock);
		Modifiers control;
		control.control = true;
		driver.key(InputID::K, control);
		CHECK_EQUAL(1u, scene.document.tileCount());
		CHECK_EQUAL("You can't clear a alpha locked layer", scene.root.lastNotice());

		scene.run(Command::ToggleLayerAlphaLock);
		scene.run(Command::ToggleLayerLock);
		scene.run(Command::ClearLayer);
		CHECK_EQUAL(1u, scene.document.tileCount());
		CHECK_EQUAL("You can't edit a locked layer", scene.root.lastNotice());

		scene.run(Command::ToggleLayerLock);
		driver.key(InputID::K, control);
		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK_EQUAL("Clear layer", scene.document.history().undoLabel());
	}

	TEST(CtrlEMergesDown) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NewLayer);
		Modifiers control;
		control.control = true;
		driver.key(InputID::E, control);
		CHECK_EQUAL(1u, scene.document.layerCount());
	}
}

namespace {
	class CountingThumbnails : public ThumbnailProvider {
	public:
		[[nodiscard]] ui::ImageId thumbnail(const Document&, const Layer& layer) override {
			requests++;
			return { 100 + layer.id() };
		}

		int requests = 0;
	};
}

SUITE(LayerPanel) {
	TEST(RowsListTopLayerFirstWithDetails) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NewLayer);
		scene.run(Command::NextLayerBlend);
		scene.run(Command::LayerOpacity60);
		scene.run(Command::ToggleLayerLock);
		LayersPopup& popup = scene.root.toolBar.layersPopup;

		CHECK_EQUAL("Layer 2", popup.rows()[0]->name.text());
		CHECK_EQUAL("Layer 1", popup.rows()[1]->name.text());
		CHECK_EQUAL("Multiply 60%, Locked", popup.rows()[0]->details.text());
		CHECK_EQUAL("Normal 100%", popup.rows()[1]->details.text());
	}

	TEST(BlendDropdownAndOpacitySliderFollowActiveLayer) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		LayersPopup& popup = scene.root.toolBar.layersPopup;
		CHECK_EQUAL(11u, popup.blend.options().size());
		CHECK_EQUAL("Soft dodge", popup.blend.options()[8]);

		scene.run(Command::NextLayerBlend);
		scene.root.toolBar.layersPopup.refresh();
		CHECK_EQUAL(1, popup.blend.selectedIndex());

		driver.click(scene.root.toolBar.layers);
		driver.click(popup.blend);
		driver.click(*popup.blend.menu().items()[4]);
		CHECK(scene.document.activeLayer()->blend() == BlendMode::Lighten);
		CHECK_EQUAL("Lighten 100%", popup.rows()[0]->details.text());
		CHECK_EQUAL("Layer blend mode", scene.document.history().undoLabel());
	}

	TEST(OpacitySliderDragIsOneHistoryStep) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		LayersPopup& popup = scene.root.toolBar.layersPopup;
		driver.click(scene.root.toolBar.layers);
		const RectF slider = popup.opacity.windowRect();
		const size_t before = scene.document.history().entries().size();

		driver.drag(
			{ slider.pos.x + slider.size.x * 0.9f, slider.pos.y + slider.size.y * 0.75f },
			{ slider.pos.x + slider.size.x * 0.3f, slider.pos.y + slider.size.y * 0.75f }
		);
		CHECK(scene.document.activeLayer()->opacity() < 0.6f);
		CHECK_EQUAL(before + 1, scene.document.history().entries().size());
	}

	TEST(ThumbnailsComeFromProvider) {
		LayerScene scene;
		CountingThumbnails thumbnails;
		scene.root.toolBar.layersPopup.setThumbnails(&thumbnails);
		scene.run(Command::NewLayer);
		CHECK_EQUAL(0, thumbnails.requests);
		UiDriver driver(scene.root, { 1280, 800 });
		driver.click(scene.root.toolBar.layers);
		CHECK(thumbnails.requests >= 2);
	}

	TEST(DraggingRowReordersLayers) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		scene.run(Command::NewLayer);
		Layer* top = scene.document.activeLayer();
		LayersPopup& popup = scene.root.toolBar.layersPopup;
		driver.click(scene.root.toolBar.layers);

		ReorderableList& list = popup.list();
		const Vec2F grip = list.windowPosition() + list.gripRect(0).center();
		const Vec2F lowest = list.windowPosition() + list.gripRect(2).center() + Vec2F(0.0f, list.gripRect(2).size.y);

		driver.moveTo(grip);
		driver.press();
		driver.moveTo(lowest);
		CHECK(list.isDragging());
		CHECK_EQUAL(2, list.dropIndex());
		CHECK(scene.document.layers()[2].get() == top);
		driver.release();

		CHECK(!list.isDragging());
		CHECK(scene.document.layers()[0].get() == top);
		CHECK(scene.document.layers()[1].get() == bottom);
		CHECK_EQUAL("Move layer up/down", scene.document.history().undoLabel());
		CHECK(&popup.rows()[2]->layer() == top);
	}

	TEST(DraggingRowBodyReordersToo) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		Layer* bottom = scene.document.activeLayer();
		scene.run(Command::NewLayer);
		LayersPopup& popup = scene.root.toolBar.layersPopup;
		driver.click(scene.root.toolBar.layers);

		driver.drag(driver.centerOf(popup.rows()[1]->name), driver.centerOf(popup.rows()[0]->name) - Vec2F(0, 12), 6);
		CHECK(scene.document.layers()[1].get() == bottom);
		CHECK(scene.document.activeLayer() == bottom);
	}

	TEST(ShortDragIsStillAClick) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NewLayer);
		LayersPopup& popup = scene.root.toolBar.layersPopup;
		driver.click(scene.root.toolBar.layers);
		Layer* bottom = scene.document.layers()[0].get();

		const Vec2F start = driver.centerOf(popup.rows()[1]->name);
		driver.drag(start, start + Vec2F(2, 1), 2);
		CHECK(scene.document.activeLayer() == bottom);
		CHECK(scene.document.layers()[0].get() == bottom);
	}

	TEST(LayerMenuListsReferenceEntries) {
		LayerScene scene;
		scene.root.layerMenu.refresh();
		std::vector<std::string> texts;
		for (const auto& item : scene.root.layerMenu.items())
			texts.push_back(item->text());

		for (const char* expected : { "New layer above current",
																	"Ascend",
																	"Descend",
																	"Duplicate",
																	"Rename",
																	"Delete",
																	"Clear",
																	"Merge",
																	"Merge all layers",
																	"Visible",
																	"Clipping mask",
																	"Locked",
																	"Locked alpha",
																	"Blend mode",
																	"Opacity" }) {
			CHECK(std::find(texts.begin(), texts.end(), expected) != texts.end());
		}
	}
}

SUITE(LayerThumbnails) {
	TEST(ThumbnailShowsLayerPixelsScaledToDocumentBounds) {
		Document document;
		uint1* pixels = document.activeLayer()->tileAt({ 0, 0 })->pixels();
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize / 2; index++) {
			pixels[index * 4 + 0] = 255;
			pixels[index * 4 + 3] = 255;
		}

		const RectF bounds = LayerThumbnail::documentBounds(document);
		CHECK_CLOSE(kTileSize, bounds.size.x, 0.01f);
		const std::vector<uint1> thumbnail = LayerThumbnail::render(*document.activeLayer(), bounds);
		CHECK_EQUAL((size_t) LayerThumbnail::kSize * LayerThumbnail::kSize * 4, thumbnail.size());
		CHECK_EQUAL(255, (int) thumbnail[0]);
		CHECK_EQUAL(255, (int) thumbnail[3]);
		const size_t lastRow = (size_t) (LayerThumbnail::kSize - 1) * LayerThumbnail::kSize * 4;
		CHECK_EQUAL(0, (int) thumbnail[lastRow + 3]);
	}

	TEST(BoundsAreSquareAroundAllTiles) {
		Document document;
		document.activeLayer()->tileAt({ 0, 0 });
		document.activeLayer()->tileAt({ 3, 1 });
		const RectF bounds = LayerThumbnail::documentBounds(document);
		CHECK_CLOSE(kTileSize * 4, bounds.size.x, 0.01f);
		CHECK_CLOSE(kTileSize * 4, bounds.size.y, 0.01f);
		CHECK_CLOSE(0, bounds.pos.x, 0.01f);
		CHECK_CLOSE(-kTileSize, bounds.pos.y, 0.01f);
	}
}

SUITE(LayerMoves) {
	TEST(MovingSeveralStepsShiftsLayersBetween) {
		Document document;
		Layer* a = document.activeLayer();
		Layer* b = document.addLayer("b");
		Layer* c = document.addLayer("c");
		Layer* d = document.addLayer("d");

		document.setActiveLayer(d);
		CHECK(document.moveActiveLayer(-3));
		CHECK(document.layers()[0].get() == d);
		CHECK(document.layers()[1].get() == a);
		CHECK(document.layers()[2].get() == b);
		CHECK(document.layers()[3].get() == c);

		CHECK(document.moveActiveLayer(2));
		CHECK(document.layers()[0].get() == a);
		CHECK(document.layers()[1].get() == b);
		CHECK(document.layers()[2].get() == d);
		CHECK(document.layers()[3].get() == c);
		CHECK(!document.moveActiveLayer(2));
	}
}

SUITE(LayerPanelRefresh) {
	TEST(StrokesAndUndoRefreshThumbnails) {
		LayerScene scene;
		CountingThumbnails thumbnails;
		scene.root.toolBar.layersPopup.setThumbnails(&thumbnails);
		UiDriver driver(scene.root, { 1280, 800 });
		const RectF canvas = scene.root.canvas.windowRect();
		const int closed = thumbnails.requests;
		driver.drag(canvas.pos + Vec2F(100, 100), canvas.pos + Vec2F(160, 120));
		CHECK_EQUAL(closed, thumbnails.requests);
		CHECK(scene.root.toolBar.layersPopup.hasStaleThumbnails());
		driver.click(scene.root.toolBar.layers);
		CHECK(thumbnails.requests > closed);
		CHECK(!scene.root.toolBar.layersPopup.hasStaleThumbnails());
		REQUIRE CHECK(scene.root.toolBar.layers.isPopupOpen());
		const int afterOpen = thumbnails.requests;
		Modifiers control;
		control.control = true;
		driver.key(InputID::Z, control);
		CHECK(thumbnails.requests > afterOpen);

		const int idle = thumbnails.requests;
		driver.moveTo(canvas.pos + Vec2F(300, 300));
		CHECK_EQUAL(idle, thumbnails.requests);
	}
}

SUITE(LayerErrorBalloons) {
	TEST(PaintingLockedLayerShowsBalloon) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::ToggleLayerLock);

		const RectF canvas = scene.root.canvas.windowRect();
		driver.drag(canvas.pos + Vec2F(100, 100), canvas.pos + Vec2F(160, 120));
		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK(scene.root.notice.isShown());
		CHECK_EQUAL("You can't edit a locked layer", scene.root.notice.text());
		CHECK(!scene.root.canvas.isPainting());

		driver.advance(3000);
		CHECK(!scene.root.notice.isShown());
	}

	TEST(ErasingAlphaLockedOrEmptyLayerShowsBalloon) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const RectF canvas = scene.root.canvas.windowRect();
		driver.key(InputID::E);

		driver.drag(canvas.pos + Vec2F(100, 100), canvas.pos + Vec2F(160, 120));
		CHECK_EQUAL("You can't erase a empty layer", scene.root.notice.text());

		fillLayer(*scene.document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		scene.run(Command::ToggleLayerAlphaLock);
		driver.drag(canvas.pos + Vec2F(100, 100), canvas.pos + Vec2F(160, 120));
		CHECK_EQUAL("You can't erase on a alpha locked layer", scene.root.notice.text());
	}

	TEST(DeletingLockedLayerShowsBalloon) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::NewLayer);
		scene.run(Command::ToggleLayerLock);
		scene.run(Command::DeleteLayer);
		CHECK(scene.root.notice.isShown());
		CHECK_EQUAL("You can't delete a locked layer", scene.root.notice.text());
	}

	TEST(BalloonDoesNotStealShortcuts) {
		LayerScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.run(Command::ToggleLayerLock);
		const RectF canvas = scene.root.canvas.windowRect();
		driver.drag(canvas.pos + Vec2F(100, 100), canvas.pos + Vec2F(160, 120));

		Modifiers control;
		control.control = true;
		driver.key(InputID::Z, control);
		CHECK(!scene.document.activeLayer()->locked());
	}
}
