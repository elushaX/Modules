#include "TestSketchesRoot.hpp"
#include "Document.hpp"
#include "History.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;

namespace {
	Brush solidBrush() {
		Brush brush;
		brush.color = { 1, 0, 0, 1 };
		brush.setFlow(1);
		brush.setHardness(1);
		brush.setSize(40);
		return brush;
	}

	int4 alphaAt(const Document& document, int4 x, int4 y) {
		document.settlePaint();
		const Layer* layer = document.activeLayer();
		const TileCoord coord = { x / kTileSize, y / kTileSize };
		const Tile* tile = layer->tileIfExists(coord);
		if (!tile) return 0;
		return tile->pixels()[((ualni) (y % kTileSize) * kTileSize + (x % kTileSize)) * 4 + 3];
	}

	void paintAt(Document& document, Vec2F point) {
		document.stampBrushDab(point, solidBrush());
		document.finishStroke("Paint");
		document.settlePaint();
	}
}

SUITE(HistoryModel) {
	TEST(CommitUndoRedoWalkStates) {
		History<int> history;
		history.reset("New file", 0);
		history.commit("A", 1);
		history.commit("B", 2);

		CHECK_EQUAL("B", history.undoLabel());
		CHECK_EQUAL(1, *history.undo());
		CHECK_EQUAL("B", history.redoLabel());
		CHECK_EQUAL(0, *history.undo());
		CHECK(!history.canUndo());
		CHECK(history.undo() == nullptr);
		CHECK_EQUAL(2, *history.jumpTo(2));
		CHECK(!history.canRedo());
	}

	TEST(CommitAfterUndoDropsRedoBranch) {
		History<int> history;
		history.reset("New file", 0);
		history.commit("A", 1);
		history.commit("B", 2);
		history.undo();
		history.commit("C", 3);

		CHECK_EQUAL(3u, history.entries().size());
		CHECK_EQUAL("C", history.entries().back().label);
		CHECK(!history.canRedo());
	}

	TEST(MergeKeyCollapsesConsecutiveCommits) {
		History<int> history;
		history.reset("New file", 0);
		history.commit("Layer opacity", 1, "layer_opacity");
		history.commit("Layer opacity", 2, "layer_opacity");
		history.commit("Layer opacity", 3, "layer_opacity");

		CHECK_EQUAL(2u, history.entries().size());
		CHECK_EQUAL(3, history.current().state);
		CHECK_EQUAL(0, *history.undo());
	}

	TEST(LimitClampsAndDropsOldestStates) {
		History<int> history;
		CHECK_EQUAL(30, history.limit());

		history.setLimit(1);
		CHECK_EQUAL(2, history.limit());
		history.setLimit(5000);
		CHECK_EQUAL(1000, history.limit());

		history.setLimit(3);
		history.reset("New file", 0);
		for (int step = 1; step <= 10; step++)
			history.commit("Step", step);

		CHECK_EQUAL(4u, history.entries().size());
		CHECK_EQUAL(7, history.entries().front().state);
		CHECK_EQUAL(10, history.current().state);
	}

	TEST(LoweringLimitTrimsExistingHistory) {
		History<int> history;
		history.reset("New file", 0);
		for (int step = 1; step <= 10; step++)
			history.commit("Step", step);

		history.setLimit(2);
		CHECK_EQUAL(3u, history.entries().size());
		CHECK_EQUAL(10, history.current().state);
	}

	TEST(ClearKeepsOnlyCurrentState) {
		History<int> history;
		history.reset("New file", 0);
		history.commit("A", 1);
		history.commit("B", 2);
		history.undo();

		history.clear();
		CHECK_EQUAL(1u, history.entries().size());
		CHECK_EQUAL(1, history.current().state);
		CHECK(!history.canUndo());
		CHECK(!history.canRedo());
	}
}

SUITE(DocumentHistory) {
	TEST(UndoRemovesStrokeAndRedoRestoresIt) {
		Document document;
		paintAt(document, { 100, 100 });
		CHECK_EQUAL(255, alphaAt(document, 100, 100));
		CHECK_EQUAL("Paint", document.history().undoLabel());

		CHECK(document.undo());
		CHECK_EQUAL(0, alphaAt(document, 100, 100));
		CHECK_EQUAL(0u, document.tileCount());

		CHECK(document.redo());
		CHECK_EQUAL(255, alphaAt(document, 100, 100));
	}

	TEST(SnapshotsShareUnchangedTilesAndCopyChangedOnes) {
		Document document;
		const int4 quarter = kTileSize / 4;
		paintAt(document, { (halnf) quarter, (halnf) quarter });
		const Tile* before = document.activeLayer()->tileIfExists({ 0, 0 });

		paintAt(document, { (halnf) (kTileSize * 3), (halnf) quarter });
		CHECK(document.activeLayer()->tileIfExists({ 0, 0 }) == before);

		paintAt(document, { (halnf) (quarter * 3), (halnf) quarter });
		CHECK(document.activeLayer()->tileIfExists({ 0, 0 }) != before);

		document.undo();
		CHECK(document.activeLayer()->tileIfExists({ 0, 0 }) == before);
		CHECK_EQUAL(0, alphaAt(document, quarter * 3 + 4, quarter));
		CHECK_EQUAL(255, alphaAt(document, quarter, quarter));
	}

	TEST(UndoRestoresLayerStackAndActiveLayer) {
		Document document;
		Layer* first = document.activeLayer();
		document.addLayer(document.nextLayerName());
		document.commit("Create layer");
		CHECK_EQUAL(2u, document.layerCount());
		CHECK_EQUAL("Layer 2", document.activeLayer()->name());

		document.removeActiveLayer();
		document.commit("Delete layer");
		CHECK_EQUAL(1u, document.layerCount());

		document.undo();
		CHECK_EQUAL(2u, document.layerCount());
		CHECK_EQUAL("Layer 2", document.activeLayer()->name());

		document.undo();
		CHECK_EQUAL(1u, document.layerCount());
		CHECK(document.activeLayer() == first);
	}

	TEST(UndoRestoresLayerProperties) {
		Document document;
		Layer& layer = *document.activeLayer();
		document.setLayerOpacity(layer, 0.4f);
		document.commit("Layer opacity", "layer_opacity");
		document.setLayerVisible(layer, false);
		document.commit("Show/hide layer");

		document.undo();
		CHECK(layer.visible());
		CHECK_CLOSE(0.4f, layer.opacity(), 0.001f);
		document.undo();
		CHECK_CLOSE(1.0f, layer.opacity(), 0.001f);
	}

	TEST(EraseUndoBringsTilesBack) {
		Document document;
		paintAt(document, { 100, 100 });

		Brush eraser = solidBrush();
		eraser.type = BrushType::Eraser;
		eraser.setSize(80);
		document.stampBrushDab({ 100, 100 }, eraser);
		document.finishStroke("Eraser");
		CHECK_EQUAL(0u, document.tileCount());

		document.undo();
		CHECK_EQUAL(1u, document.tileCount());
		CHECK_EQUAL(255, alphaAt(document, 100, 100));
	}

	TEST(JumpToHistoryState) {
		Document document;
		paintAt(document, { 100, 100 });
		paintAt(document, { 700, 100 });
		paintAt(document, { 1300, 100 });

		document.jumpToHistory(1);
		CHECK_EQUAL(1u, document.tileCount());
		document.jumpToHistory(3);
		CHECK_EQUAL(3u, document.tileCount());
	}

	TEST(ResetStartsFreshHistory) {
		Document document;
		paintAt(document, { 100, 100 });
		document.addLayer(document.nextLayerName());
		document.commit("Create layer");

		document.reset();
		CHECK_EQUAL(1u, document.layerCount());
		CHECK_EQUAL("Layer 1", document.activeLayer()->name());
		CHECK_EQUAL(1u, document.history().entries().size());
		CHECK_EQUAL("New file", document.history().current().label);
	}
}

#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class QuietViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class PlainIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct HistoryScene {
		Document document;
		Brush brush;
		QuietViewport viewport;
		PlainIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
	};

	Modifiers ctrlKey(bool shift = false) {
		Modifiers modifiers;
		modifiers.control = true;
		modifiers.shift = shift;
		return modifiers;
	}

	const MenuItem& editItem(Sketch2DRoot& root, size_t index) { return *root.editMenu.items()[index]; }

	void strokeOnCanvas(UiDriver& driver, Sketch2DRoot& root, Vec2F offset) {
		const RectF canvas = root.canvas.windowRect();
		driver.drag(canvas.pos + offset, canvas.pos + offset + Vec2F(60, 20));
	}
}

SUITE(UndoRedoCommands) {
	TEST(ShortcutsUndoAndRedoStrokes) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		strokeOnCanvas(driver, scene.root, { 100, 100 });
		CHECK(scene.document.tileCount() > 0);

		driver.key(InputID::Z, ctrlKey());
		CHECK_EQUAL(0u, scene.document.tileCount());

		driver.key(InputID::Z, ctrlKey(true));
		CHECK(scene.document.tileCount() > 0);

		driver.key(InputID::Y, ctrlKey());
		CHECK_EQUAL(0u, scene.document.tileCount());

		driver.key(InputID::Y, ctrlKey(true));
		CHECK(scene.document.tileCount() > 0);
	}

	TEST(HoldingUndoRepeats) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		for (int stroke = 0; stroke < 4; stroke++)
			strokeOnCanvas(driver, scene.root, Vec2F(100.0f + 150.0f * (halnf) stroke, 100.0f));
		CHECK_EQUAL(4u, scene.document.history().currentIndex());

		driver.holdKey(InputID::Z, 2, ctrlKey());
		CHECK_EQUAL(1u, scene.document.history().currentIndex());
	}

	TEST(EditMenuNamesTheStepAndDisablesAtEnds) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.editMenu.refresh();
		CHECK_EQUAL("Undo", editItem(scene.root, 0).text());
		CHECK(!editItem(scene.root, 0).isEnabled());
		CHECK_EQUAL("Redo", editItem(scene.root, 1).text());

		strokeOnCanvas(driver, scene.root, { 100, 100 });
		driver.key(InputID::N, ctrlKey(true));
		scene.root.editMenu.refresh();
		CHECK_EQUAL("Undo: Create layer", editItem(scene.root, 0).text());
		CHECK(editItem(scene.root, 0).isEnabled());

		driver.key(InputID::Z, ctrlKey());
		scene.root.editMenu.refresh();
		CHECK_EQUAL("Undo: Paint", editItem(scene.root, 0).text());
		CHECK_EQUAL("Redo: Create layer", editItem(scene.root, 1).text());
	}

	TEST(EraserStrokeIsLabelledEraser) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		strokeOnCanvas(driver, scene.root, { 100, 100 });
		driver.key(InputID::E);
		strokeOnCanvas(driver, scene.root, { 100, 100 });

		CHECK_EQUAL("Eraser", scene.document.history().undoLabel());
	}

	TEST(ToolbarButtonsFollowHistory) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		CHECK(!scene.root.toolBar.undo.isEnabled());
		CHECK(!scene.root.toolBar.redo.isEnabled());

		strokeOnCanvas(driver, scene.root, { 100, 100 });
		CHECK(scene.root.toolBar.undo.isEnabled());

		driver.click(scene.root.toolBar.undo);
		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK(scene.root.toolBar.redo.isEnabled());
		CHECK(!scene.root.toolBar.undo.isEnabled());

		driver.click(scene.root.toolBar.redo);
		CHECK(scene.document.tileCount() > 0);
	}

	TEST(UndoRefreshesLayerPanel) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.key(InputID::N, ctrlKey(true));
		CHECK_EQUAL(2u, scene.root.toolBar.layersPopup.rows().size());

		driver.key(InputID::Z, ctrlKey());
		CHECK_EQUAL(1u, scene.root.toolBar.layersPopup.rows().size());
	}
}

SUITE(HistoryMenu) {
	TEST(ListsStatesNewestFirstWithCurrentChecked) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		strokeOnCanvas(driver, scene.root, { 100, 100 });
		driver.key(InputID::N, ctrlKey(true));
		driver.key(InputID::Z, ctrlKey());

		scene.root.historyMenu.refresh();
		const auto& items = scene.root.historyMenu.items();
		CHECK_EQUAL("Create layer", items[0]->text());
		CHECK_EQUAL("Paint", items[1]->text());
		CHECK_EQUAL("New file", items[2]->text());
		CHECK(!items[0]->entry().isChecked());
		CHECK(items[1]->entry().isChecked());
		CHECK(items[0]->entry().muted);
		CHECK(!items[2]->entry().muted);
		CHECK_EQUAL("Clear history", items[3]->text());
		CHECK_EQUAL("Set history limit", items[4]->text());
	}

	TEST(ClickingStateJumpsThere) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		strokeOnCanvas(driver, scene.root, { 100, 100 });
		strokeOnCanvas(driver, scene.root, { 400, 100 });

		for (const auto& title : scene.root.menuBar.titles())
			if (title->text() == "History") driver.click(*title);
		CHECK(scene.root.historyMenu.isOpen());
		driver.click(*scene.root.historyMenu.items()[2]);
		CHECK_EQUAL(0u, scene.document.history().currentIndex());
		CHECK_EQUAL(0u, scene.document.tileCount());
	}

	TEST(LongHistoryShowsMoreMarkers) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		for (int step = 0; step < 25; step++) {
			scene.document.addLayer(scene.document.nextLayerName());
			scene.document.commit("Create layer");
		}
		scene.document.jumpToHistory(15);

		scene.root.historyMenu.refresh();
		const auto& items = scene.root.historyMenu.items();
		CHECK_EQUAL("(4 More)", items.front()->text());
		CHECK(!items.front()->isEnabled());
		CHECK_EQUAL(1u + 6u + 1u + 12u + 1u + 2u, items.size());
		CHECK_EQUAL("(3 More)", items[items.size() - 3]->text());
	}
}

SUITE(HistoryDialogs) {
	TEST(ClearHistoryAsksAndKeepsCurrentState) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		strokeOnCanvas(driver, scene.root, { 100, 100 });
		strokeOnCanvas(driver, scene.root, { 400, 100 });
		const ualni tiles = scene.document.tileCount();

		scene.root.commands.run(Command::ClearHistory);
		CHECK(scene.root.confirm.isOpen());
		CHECK_EQUAL("Are you sure you want to clear history?", scene.root.confirm.message());
		CHECK_EQUAL(3u, scene.document.history().entries().size());

		driver.click(*scene.root.confirm.buttons()[0]);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
		CHECK_EQUAL(tiles, scene.document.tileCount());
		CHECK(!scene.root.commands.isEnabled(Command::Undo));
		CHECK(!scene.root.commands.isEnabled(Command::ClearHistory));
	}

	TEST(CancellingClearKeepsHistory) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		strokeOnCanvas(driver, scene.root, { 100, 100 });

		scene.root.commands.run(Command::ClearHistory);
		driver.key(InputID::ESCAPE);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
	}

	TEST(SetHistoryLimitDialogAppliesClampedValue) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.commands.run(Command::SetHistoryLimit);
		CHECK(scene.root.historyLimit.isOpen());
		CHECK_EQUAL("30", scene.root.historyLimit.steps.field().text());

		driver.click(scene.root.historyLimit.steps.field());
		driver.type("5");
		driver.key(InputID::ENTER);
		driver.click(*scene.root.historyLimit.buttons()[0]);
		CHECK_EQUAL(5, scene.document.history().limit());

		scene.root.commands.run(Command::SetHistoryLimit);
		driver.click(scene.root.historyLimit.steps.decrementButton());
		driver.click(scene.root.historyLimit.steps.decrementButton());
		driver.click(scene.root.historyLimit.steps.decrementButton());
		driver.click(*scene.root.historyLimit.buttons()[0]);
		CHECK_EQUAL(2, scene.document.history().limit());
	}

	TEST(CancelLeavesLimitUnchanged) {
		HistoryScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.commands.run(Command::SetHistoryLimit);
		driver.click(scene.root.historyLimit.steps.incrementButton());
		driver.click(*scene.root.historyLimit.buttons()[1]);
		CHECK_EQUAL(30, scene.document.history().limit());
	}
}
