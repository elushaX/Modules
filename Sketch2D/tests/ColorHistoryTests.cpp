#include "TestSketchesRoot.hpp"
#include "ColorModel.hpp"
#include "Sketch2DUI.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <filesystem>
#include <fstream>
#include <unistd.h>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class HistoryViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class HistoryIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	std::filesystem::path scratchFile(const std::string& name) {
		return std::filesystem::temp_directory_path() / (name + "-" + std::to_string(::getpid()) + ".json");
	}

	struct HistoryScene {
		Document document;
		Brush brush;
		HistoryViewport viewport;
		HistoryIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		std::filesystem::path swatchFile = scratchFile("sketch2d-swatches");
		UiDriver driver{ root, { 1280, 800 } };

		HistoryScene() {
			root.canvas.settings().viewStiffness = 1;
			std::filesystem::remove(swatchFile);
			root.swatchPath = swatchFile.string();
			root.swatches = SwatchSet();
			root.toolBar.colorPopup.showSwatches(root.swatches);
			brush.apply(*findBrushPreset(BrushType::Round));
		}

		~HistoryScene() { std::filesystem::remove(swatchFile); }

		void paintWith(const HueColor& color, Vec2F at = { 600, 400 }) {
			brush.color = color.rgba();
			driver.drag(at, at + Vec2F{ 80, 20 });
		}

		ColorPopup& popup() { return root.toolBar.colorPopup; }

		void open() {
			root.refreshFromState();
			driver.settle();
			driver.click(root.toolBar.color);
			REQUIRE CHECK(driver.ui().isPopupOpen(popup()));
			driver.layout();
		}
	};

	void checkColor(const RGBA& expected, const RGBA& actual) {
		CHECK_CLOSE(expected.r, actual.r, 0.01f);
		CHECK_CLOSE(expected.g, actual.g, 0.01f);
		CHECK_CLOSE(expected.b, actual.b, 0.01f);
	}
}

SUITE(ColorHistoryModel) {
	TEST(NewestColorComesFirstAndRepeatsCount) {
		ColorHistory history;
		history.recordStroke({ 0, 1, 1 });
		history.recordStroke({ 120, 1, 1 });
		history.recordStroke({ 0, 1, 1 });
		REQUIRE CHECK_EQUAL((size_t) 2, history.entries().size());
		CHECK_CLOSE(0.0f, history.entries()[0].color.hueDegrees, 1e-3f);
		CHECK_EQUAL(2, history.entries()[0].strokes);
		CHECK_EQUAL(1, history.entries()[1].strokes);
	}

	TEST(KeepsTheReferenceSevenColors) {
		ColorHistory history;
		for (int4 index = 0; index < 10; index++)
			history.recordStroke({ (halnf) index * 30, 1, 1 });
		REQUIRE CHECK_EQUAL(ColorHistory::kCapacity, history.entries().size());
		CHECK_EQUAL((size_t) 7, ColorHistory::kCapacity);
		CHECK_CLOSE(270.0f, history.entries().front().color.hueDegrees, 1e-3f);
		CHECK_CLOSE(90.0f, history.entries().back().color.hueDegrees, 1e-3f);
	}

	TEST(ColorsMatchByTheirHexValue) {
		ColorHistory history;
		history.recordStroke({ 10, 0.5f, 0.5f });
		history.recordStroke({ 10.0001f, 0.5f, 0.5f });
		CHECK_EQUAL((size_t) 1, history.entries().size());
		CHECK_EQUAL(2, history.entries().front().strokes);
	}
}

SUITE(SwatchModel) {
	TEST(SwatchesSkipDuplicatesAndRemove) {
		SwatchSet set;
		CHECK(set.add({ 1, 0, 0, 1 }));
		CHECK(!set.add({ 1, 0, 0, 1 }));
		CHECK(set.add({ 0, 1, 0, 1 }));
		set.remove(0);
		REQUIRE CHECK_EQUAL((size_t) 1, set.colors().size());
		CHECK_CLOSE(1.0f, set.colors()[0].g, 1e-6f);
		set.remove(5);
		CHECK_EQUAL((size_t) 1, set.colors().size());
	}

	TEST(SwatchesStopAtCapacity) {
		SwatchSet set;
		for (size_t index = 0; index < SwatchSet::kCapacity; index++)
			CHECK(set.add({ (halnf) index / 255.0f, 0, 0, 1 }));
		CHECK(!set.add({ 0, 0, 1, 1 }));
	}

	TEST(JsonUsesTheReferenceFormat) {
		SwatchSet set;
		set.add({ 1, 0.5f, 0, 1 });
		set.add({ 0, 0, 1, 1 });
		const std::string json = set.toJson(11, 22);
		CHECK(json.find("\"object\" : \"swatches\"") != std::string::npos);
		CHECK(json.find("\"created\" : \"11\"") != std::string::npos);
		CHECK(json.find("\"modified\" : \"22\"") != std::string::npos);
		CHECK(json.find("{ \"hex\" : \"#FF8000\" }") != std::string::npos);
		const auto back = SwatchSet::fromJson(json);
		REQUIRE CHECK(back.has_value());
		REQUIRE CHECK_EQUAL((size_t) 2, back->colors().size());
		CHECK_CLOSE(1.0f, back->colors()[1].b, 1e-6f);
	}

	TEST(BrokenJsonIsRejected) {
		CHECK(!SwatchSet::fromJson("").has_value());
		CHECK(!SwatchSet::fromJson("{ \"object\" : \"brushes\", \"data\" : [] }").has_value());
		CHECK(!SwatchSet::fromJson("{ \"object\" : \"swatches\" }").has_value());
		CHECK(!SwatchSet::fromJson("{ \"object\" : \"swatches\", \"data\" : [ { \"hex\" : \"#zz\" } ] }").has_value());
		const auto empty = SwatchSet::fromJson("{ \"object\" : \"swatches\", \"data\" : [ ] }");
		REQUIRE CHECK(empty.has_value());
		CHECK(empty->colors().empty());
	}

	TEST(SwatchesSaveAndLoad) {
		const std::filesystem::path file = scratchFile("sketch2d-swatch-roundtrip");
		SwatchSet set;
		set.add({ 0.2f, 0.4f, 0.6f, 1 });
		REQUIRE CHECK(set.save(file.string()));
		const SwatchSet loaded = SwatchSet::load(file.string());
		REQUIRE CHECK_EQUAL((size_t) 1, loaded.colors().size());
		CHECK_EQUAL(colorHex({ 0.2f, 0.4f, 0.6f, 1 }), colorHex(loaded.colors()[0]));
		std::filesystem::remove(file);
		CHECK(SwatchSet::load(file.string()).colors().empty());
	}
}

SUITE(ColorHistoryPanel) {
	TEST(PaintingRecordsHistory) {
		HistoryScene scene;
		scene.paintWith({ 0, 1, 1 });
		scene.paintWith({ 120, 1, 1 });
		scene.paintWith({ 0, 1, 1 });
		REQUIRE CHECK_EQUAL((size_t) 2, scene.root.colorHistory.entries().size());
		CHECK_EQUAL(2, scene.root.colorHistory.entries()[0].strokes);

		scene.open();
		SwatchGrid& grid = scene.popup().history;
		REQUIRE CHECK_EQUAL((size_t) 2, grid.colors().size());
		checkColor(HueColor{ 0, 1, 1 }.rgba(), grid.colors()[0]);
		bool countShown = false;
		for (const PaintCommand& command : scene.driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Text && command.text == "2") countShown = true;
		CHECK(countShown);
	}

	TEST(ErasingAndAdjustingDoNotRecordHistory) {
		HistoryScene scene;
		scene.paintWith({ 0, 1, 1 });
		scene.root.colorHistory.clear();
		const size_t before = scene.document.history().revision();
		scene.brush.color = HueColor{ 120, 1, 1 }.rgba();
		scene.brush.setEraserMode(true);
		REQUIRE CHECK(scene.brush.erases());
		scene.driver.drag({ 600, 400 }, { 680, 420 });
		CHECK(scene.document.history().revision() > before);
		CHECK(scene.root.colorHistory.entries().empty());
		scene.brush.setEraserMode(false);
		scene.root.canvas.setAdjustTool(BrushParameter::Size, true);
		scene.driver.drag({ 600, 400 }, { 650, 400 });
		CHECK(scene.root.colorHistory.entries().empty());
	}

	TEST(TappingHistoryPicksTheColor) {
		HistoryScene scene;
		scene.paintWith({ 200, 0.7f, 0.6f });
		scene.paintWith({ 40, 1, 1 });
		scene.open();
		SwatchGrid& grid = scene.popup().history;
		scene.driver.click(grid.windowRect().pos + grid.cellRect(1).center());
		checkColor(HueColor{ 200, 0.7f, 0.6f }.rgba(), scene.brush.color);
		CHECK_CLOSE(200.0f, scene.root.colors.primary().hueDegrees, 0.5f);
	}

	TEST(ClearHistoryEmptiesThePage) {
		HistoryScene scene;
		scene.paintWith({ 0, 1, 1 });
		scene.root.commands.run(Command::ClearColorHistory);
		CHECK(scene.root.colorHistory.entries().empty());
		CHECK(scene.popup().history.colors().empty());
	}
}

SUITE(ColorPopupSections) {
	TEST(HistoryAndSwatchesSitBelowThePickerOnEveryPage) {
		HistoryScene scene;
		scene.paintWith({ 0, 1, 1 });
		scene.root.commands.run(Command::AddSwatch);
		scene.open();
		ColorPopup& popup = scene.popup();
		CHECK_EQUAL((size_t) ColorPage::Count, popup.pages.tabCount());
		for (size_t page = 0; page < (size_t) ColorPage::Count; page++) {
			popup.showPage((ColorPage) page);
			scene.driver.layout();
			REQUIRE CHECK(popup.history.isVisible());
			REQUIRE CHECK(popup.swatches.isVisible());
			const RectF picker = popup.pages.windowRect();
			const RectF history = popup.history.windowRect();
			const RectF swatches = popup.swatches.windowRect();
			const RectF box = popup.windowRect();
			CHECK(history.pos.y >= picker.pos.y + picker.size.y);
			CHECK(swatches.pos.y >= history.pos.y + history.size.y);
			CHECK(swatches.pos.y + swatches.size.y <= box.pos.y + box.size.y + 0.5f);
			CHECK(box.pos.y >= -0.5f);
			CHECK(box.pos.y + box.size.y <= 800.5f);
		}
	}

	TEST(EmptySectionsStayOutOfTheWay) {
		HistoryScene scene;
		scene.open();
		CHECK(!scene.popup().history.isVisible());
		CHECK(!scene.popup().historyTitle.isVisible());
		CHECK(!scene.popup().swatches.isVisible());
		CHECK(scene.popup().swatchHeader.isVisible());
		CHECK_EQUAL(std::string("Tap + to keep the current color here"), scene.popup().swatchHint.text());
	}
}

SUITE(SwatchPanel) {
	TEST(AddButtonKeepsTheColorAndSavesIt) {
		HistoryScene scene;
		scene.brush.color = HueColor{ 30, 0.8f, 0.9f }.rgba();
		scene.open();
		scene.driver.click(scene.popup().addSwatch);
		REQUIRE CHECK_EQUAL((size_t) 1, scene.popup().swatches.colors().size());
		checkColor(scene.brush.color, scene.popup().swatches.colors()[0]);
		CHECK(std::filesystem::exists(scene.swatchFile));
		CHECK_EQUAL((size_t) 1, SwatchSet::load(scene.swatchFile.string()).colors().size());

		scene.driver.click(scene.popup().addSwatch);
		CHECK_EQUAL(std::string("Already in swatches"), scene.root.lastNotice());
		CHECK_EQUAL((size_t) 1, scene.popup().swatches.colors().size());
	}

	TEST(TappingASwatchPicksItAndHoldingRemovesIt) {
		HistoryScene scene;
		scene.brush.color = HueColor{ 0, 1, 1 }.rgba();
		scene.root.commands.run(Command::AddSwatch);
		scene.brush.color = HueColor{ 240, 1, 1 }.rgba();
		scene.root.commands.run(Command::AddSwatch);
		scene.open();
		SwatchGrid& grid = scene.popup().swatches;
		REQUIRE CHECK_EQUAL((size_t) 2, grid.colors().size());

		scene.driver.click(grid.windowRect().pos + grid.cellRect(0).center());
		checkColor({ 1, 0, 0, 1 }, scene.brush.color);

		scene.driver.moveTo(grid.windowRect().pos + grid.cellRect(0).center());
		scene.driver.press();
		scene.driver.advance(Ui::kLongPressMs + 20);
		scene.driver.release();
		REQUIRE CHECK_EQUAL((size_t) 1, grid.colors().size());
		checkColor({ 0, 0, 1, 1 }, grid.colors()[0]);
		CHECK_EQUAL((size_t) 1, SwatchSet::load(scene.swatchFile.string()).colors().size());
	}

	TEST(ColorMenuOffersSwatchAndHistoryCommands) {
		HistoryScene scene;
		bool add = false;
		bool clear = false;
		for (const auto& item : scene.root.colorMenu.items()) {
			if (item->entry().text == "Add color to swatches") add = true;
			if (item->entry().text == "Clear color history") clear = true;
		}
		CHECK(add);
		CHECK(clear);
	}

	TEST(TabletPickerHasHistoryAndSwatches) {
		Document document;
		Brush brush;
		HistoryViewport viewport;
		HistoryIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		const std::filesystem::path file = scratchFile("sketch2d-tablet-swatches");
		classic.swatchPath = file.string();
		classic.swatches = SwatchSet();
		StudioRoot studio{ classic, brush, icons };
		UiDriver driver{ studio, { 1280, 800 } };
		brush.apply(*findBrushPreset(BrushType::Round));
		brush.color = HueColor{ 90, 1, 1 }.rgba();
		driver.drag({ 600, 400 }, { 700, 420 });
		CHECK_EQUAL((size_t) 1, classic.colorHistory.entries().size());

		driver.click(studio.color);
		ColorPopup& popup = classic.toolBar.colorPopup;
		REQUIRE CHECK(driver.ui().isPopupOpen(popup));
		CHECK(popup.history.isVisible());
		driver.click(popup.addSwatch);
		CHECK_EQUAL((size_t) 1, popup.swatches.colors().size());
		std::filesystem::remove(file);
	}
}
