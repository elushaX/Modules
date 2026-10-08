#include "TestSketchesRoot.hpp"
#include "Sketch2DUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cstdlib>
#include <filesystem>

#include <stdexcept>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class FakeViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F viewportSize) override {
			renders++;
			lastViewport = viewportSize;
		}

		[[nodiscard]] ImageId image() const override { return { 42 }; }
		void setDebugOverlay(bool enabled) override { overlay = enabled; }
		[[nodiscard]] bool debugOverlay() const override { return overlay; }
		[[nodiscard]] std::string statistics() const override { return "fake statistics"; }

		int renders = 0;
		Vec2F lastViewport = { 0, 0 };
		bool overlay = false;
	};

	class NumberedIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct Sketch2DScene {
		Document document;
		Brush brush;
		FakeViewport viewport;
		NumberedIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };

		Sketch2DScene() { root.canvas.settings().viewStiffness = 1; }
	};

	MenuItem& itemNamed(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items()) {
			if (item->entry().text == text) return *item;
		}
		throw std::runtime_error("menu item not found: " + text);
	}

	struct FakeRecorderHost : SessionRecorderHost {
		WindowInputObserver* tap = nullptr;
		double clockMs = 1000;

		void setRawInputTap(WindowInputObserver* observer) override { tap = observer; }
		[[nodiscard]] SessionEnvironment environment() const override {
			return { "test", "fake gpu", "today", { 1280, 800 }, 1 };
		}
		[[nodiscard]] double nowMs() const override { return clockMs; }
	};

	size_t menuIndex(Sketch2DRoot& root, const std::string& title) {
		for (size_t index = 0; index < root.menuBar.titles().size(); index++) {
			if (root.menuBar.titles()[index]->text() == title) return index;
		}
		throw std::runtime_error("menu not found: " + title);
	}

	std::string percentLabel(halnf fraction) { return std::to_string((int) std::round(fraction * 100)) + "%"; }

	PopupButton& menuTitle(Sketch2DRoot& root, size_t index) { return *root.menuBar.titles()[index]; }

	bool hasPaintedPixels(Document& document) {
		document.settlePaint();
		bool painted = false;
		document.forEachVisibleTile([&painted](const TileCoord&, Tile* tile) {
			const uint1* pixels = tile->pixels();
			for (ualni index = 3; index < (ualni) kTileSize * kTileSize * 4 && !painted; index += 4) {
				painted = pixels[index] > 0;
			}
		});
		return painted;
	}
}

SUITE(Sketch2DPainting) {
	TEST(OverlappingSoftDabsStorePremultipliedColor) {
		Document document;
		Brush brush;
		brush.color = { 1.0f, 0.5f, 0.25f, 1.0f };
		brush.setSize(40);
		brush.setHardness(0.0f);
		brush.setFlow(1.0f);
		document.stampBrushDab({ 128.0f, 128.0f }, brush);
		document.stampBrushDab({ 136.0f, 128.0f }, brush);
		document.settlePaint();

		const uint1* pixels = document.activeLayer()->tileIfExists({ 0, 0 })->pixels();
		int4 softPixels = 0;
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize; index++) {
			const uint1* pixel = pixels + index * 4;
			const int4 alpha = pixel[3];
			if (alpha == 0 || alpha == 255) continue;
			softPixels++;
			CHECK(pixel[0] <= alpha && pixel[1] <= alpha && pixel[2] <= alpha);
			CHECK_CLOSE(alpha * 1.0f, pixel[0], 1.0f);
			CHECK_CLOSE(alpha * 0.5f, pixel[1], 1.0f);
			CHECK_CLOSE(alpha * 0.25f, pixel[2], 1.0f);
		}
		CHECK(softPixels > 100);
	}
}

SUITE(Sketch2DTileLifetime) {
	TEST(ErasingAStrokeRemovesItsTiles) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const RectF canvas = scene.root.canvas.windowRect();
		const Vec2F from = canvas.pos + Vec2F(100, 100);
		const Vec2F to = canvas.pos + Vec2F(700, 400);

		driver.drag(from, to);
		const ualni painted = scene.document.tileCount();
		CHECK(painted > 1);

		scene.brush.type = BrushType::Eraser;
		scene.brush.setSize(120);
		scene.brush.setFlow(1.0f);
		scene.brush.setHardness(1.0f);
		driver.drag(from, to);

		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK(!hasPaintedPixels(scene.document));
	}

	TEST(PartialEraseKeepsPaintedTiles) {
		Document document;
		Brush brush;
		brush.setFlow(1.0f);
		brush.setSize(kTileSize * 0.25f);
		const halnf middle = (halnf) kTileSize * 0.5f;
		document.stampBrushDab({ middle, middle }, brush);
		document.stampBrushDab({ (halnf) kTileSize * 2.5f, middle }, brush);
		document.finishStroke();
		CHECK_EQUAL(2u, document.tileCount());

		brush.type = BrushType::Eraser;
		brush.setSize(kTileSize * 0.5f);
		brush.setHardness(1.0f);
		document.stampBrushDab({ middle, middle }, brush);
		document.finishStroke();

		CHECK_EQUAL(1u, document.tileCount());
		CHECK(document.activeLayer()->tileIfExists({ 2, 0 }) != nullptr);
	}
}

SUITE(Sketch2DLayout) {
	TEST(PaintingRendersViewportAtCanvasSize) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		RecordingPainter& painter = driver.paint();

		CHECK_EQUAL(1, scene.viewport.renders);
		CHECK_CLOSE(scene.root.canvas.size().x, scene.viewport.lastViewport.x, 0.01f);
		CHECK(painter.hasText("File"));
		CHECK(painter.hasText("Layers"));
		CHECK(painter.hasText("10 px"));
		CHECK(painter.isBalanced());
	}

	TEST(ShrinkingWindowKeepsCanvasPositive) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.resize({ 500, 300 });

		CHECK(scene.root.canvas.size().x > 0);
		CHECK(scene.root.canvas.size().y > 0);
	}
}

SUITE(Sketch2DCanvasInteraction) {
	TEST(DraggingOnCanvasPaintsAStroke) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F start = scene.root.canvas.windowRect().center();

		driver.moveTo(start);
		driver.press();
		CHECK(scene.root.canvas.isPainting());
		driver.moveTo(start + Vec2F(120.0f, 40.0f));
		driver.release();

		CHECK(!scene.root.canvas.isPainting());
		CHECK(scene.document.tileCount() > 0);
		CHECK(hasPaintedPixels(scene.document));
	}

	TEST(StrokeContinuesWhenPointerLeavesCanvas) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F start = scene.root.canvas.windowRect().center();

		driver.moveTo(start);
		driver.press();
		driver.moveTo({ 10, 10 });

		CHECK(scene.root.canvas.isPainting());
		CHECK(driver.ui().pointerCapture() == &scene.root.canvas);
		driver.release();
	}

	TEST(MiddleDragPansWithoutPainting) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F start = scene.root.canvas.windowRect().center();

		driver.moveTo(start);
		driver.press(PointerButton::Middle);
		CHECK(scene.root.canvas.isPanning());
		driver.moveTo(start + Vec2F(100.0f, 0.0f));
		driver.release(PointerButton::Middle);

		CHECK_CLOSE(-100, scene.document.panWorld().x, 0.01f);
		CHECK_EQUAL(0u, scene.document.tileCount());
	}

	TEST(SpaceDragAlsoPans) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F start = scene.root.canvas.windowRect().center();

		driver.ui().dispatch(Event::key(Event::Type::KeyDown, InputID::SPACE));
		driver.drag(start, start + Vec2F(0.0f, 50.0f));
		driver.ui().dispatch(Event::key(Event::Type::KeyUp, InputID::SPACE));

		CHECK_CLOSE(-50, scene.document.panWorld().y, 0.01f);
		CHECK_EQUAL(0u, scene.document.tileCount());
	}

	TEST(WheelZoomsAndToolbarFollows) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.moveTo(scene.root.canvas);
		driver.wheel(1);

		CHECK_CLOSE(1.1f, scene.document.zoom(), 0.001f);
		CHECK_EQUAL("110%", scene.root.toolBar.zoom.text());

		driver.click(scene.root.toolBar.zoom);
		CHECK_CLOSE(1.0f, scene.document.zoom(), 0.001f);
		CHECK_EQUAL("100%", scene.root.toolBar.zoom.text());
	}
}

SUITE(Sketch2DToolsAndPopups) {
	TEST(EraserAndBrushToolsSwitchBrushType) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.click(scene.root.toolStrip.button("eraser"));
		CHECK(scene.brush.type == BrushType::Eraser);
		CHECK(scene.root.toolStrip.button("eraser").isSelected());
		CHECK(!scene.root.toolStrip.button("brush").isSelected());

		driver.click(scene.root.toolStrip.button("brush"));
		CHECK(scene.brush.type == BrushType::Round);
		CHECK(scene.root.toolStrip.button("brush").isSelected());
	}

	TEST(ColorPopupEraserModeErasesWithTheCurrentBrush) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.apply(*findBrushPreset(BrushType::Pencil));
		ColorPopup& popup = scene.root.toolBar.colorPopup;

		driver.click(scene.root.toolBar.color);
		REQUIRE CHECK(driver.ui().isPopupOpen(popup));
		driver.click(popup.eraserMode);
		CHECK(scene.brush.erases());
		CHECK(scene.brush.type == BrushType::Pencil);

		const RectF wheel = popup.wheel.windowRect();
		driver.click(wheel.center() + Vec2F(wheel.size.x * 0.3f, 0.0f));
		CHECK(!scene.brush.erases());
		CHECK(!popup.eraserMode.isChecked());
	}

	TEST(SmudgeToolSwapsInTheSmudgeBrushAndBack) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.apply(*findBrushPreset(BrushType::Pencil));

		driver.click(scene.root.toolStrip.button("smudge"));
		CHECK(scene.brush.type == BrushType::Smudge);
		CHECK(scene.root.toolStrip.button("smudge").isSelected());
		CHECK(!scene.root.toolStrip.button("brush").isSelected());

		driver.click(scene.root.toolStrip.button("eraser"));
		driver.click(scene.root.toolStrip.button("brush"));
		CHECK(scene.brush.type == BrushType::Pencil);
	}

	TEST(SmudgeStrokeIsLabelledInHistory) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const RectF canvas = scene.root.canvas.windowRect();
		driver.drag(canvas.center(), canvas.center() + Vec2F(60.0f, 0.0f));

		driver.click(scene.root.toolStrip.button("smudge"));
		driver.drag(canvas.center() + Vec2F(30.0f, 0.0f), canvas.center() + Vec2F(30.0f, 60.0f));
		CHECK_EQUAL("Smudge", scene.document.history().entries().back().label);
	}

	TEST(BrushSizePopupChangesRadiusAndToolbarLabel) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;

		driver.click(scene.root.toolBar.brushSize);
		REQUIRE CHECK(driver.ui().isPopupOpen(popup));
		CHECK(popup.windowRect().pos.y >= scene.root.toolBar.windowRect().pos.y + scene.root.toolBar.size().y - 1);

		const RectF slider = popup.size.slider().windowRect();
		driver.drag(slider.center(), { slider.pos.x + slider.size.x, slider.center().y });

		CHECK_CLOSE(Brush::kMaximumSize, scene.brush.size(), 0.5f);
		CHECK_EQUAL("10000 px", scene.root.toolBar.brushSize.text());
		CHECK(driver.ui().isPopupOpen(popup));

		driver.click(scene.root.canvas);
		CHECK(!driver.ui().isPopupOpen(popup));
	}

	TEST(ColorPopupPicksBrushColor) {
		Sketch2DScene scene;
		scene.brush.color = { 0.2f, 0.4f, 0.8f, 1.0f };
		UiDriver driver(scene.root, { 1280, 800 });
		const RGBA before = scene.brush.color;
		scene.brush.type = BrushType::Eraser;

		driver.click(scene.root.toolBar.color);
		const RectF wheel = scene.root.toolBar.colorPopup.wheel.windowRect();
		const halnf ring = (wheel.size.x < wheel.size.y ? wheel.size.x : wheel.size.y) / 2 - 4;
		driver.click(wheel.center() + Vec2F(ring, 0.0f));

		CHECK(!(scene.brush.color == before));
		CHECK(scene.brush.type == BrushType::Round);
	}

	TEST(LayersPopupManagesDocumentLayers) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		LayersPopup& layers = scene.root.toolBar.layersPopup;

		driver.click(scene.root.toolBar.layers);
		REQUIRE CHECK(driver.ui().isPopupOpen(layers));
		const RectF anchor = scene.root.toolBar.layers.windowRect();
		CHECK_CLOSE(anchor.pos.x + anchor.size.x, layers.windowRect().pos.x + layers.size().x, 0.5f);

		driver.click(layers.add);
		CHECK_EQUAL(2u, scene.document.layerCount());
		CHECK_EQUAL(2u, layers.rows().size());
		CHECK(scene.document.activeLayer() == &layers.rows()[0]->layer());

		driver.click(layers.rows()[1]->name);
		CHECK(scene.document.activeLayer() == &layers.rows()[1]->layer());

		Layer& bottom = layers.rows()[1]->layer();
		driver.click(layers.rows()[1]->visibility);
		CHECK(!bottom.visible());

		driver.click(layers.rows()[0]->remove);
		CHECK_EQUAL(1u, scene.document.layerCount());
		CHECK_EQUAL(1u, layers.rows().size());
		CHECK(driver.ui().isPopupOpen(layers));
	}
}

SUITE(Sketch2DMenus) {
	TEST(TileStatisticsToggleShowsOverlay) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		driver.click(menuTitle(scene.root, menuIndex(scene.root, "Debug")));
		driver.click(itemNamed(scene.root.debugMenu, "Show Tile Statistics"));

		CHECK(scene.viewport.overlay);
		CHECK(driver.paint().hasText("fake statistics"));
	}

	TEST(ClearCanvasRemovesStroke) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F start = scene.root.canvas.windowRect().center();
		driver.drag(start, start + Vec2F(50.0f, 0.0f));
		REQUIRE CHECK(hasPaintedPixels(scene.document));

		driver.click(menuTitle(scene.root, menuIndex(scene.root, "Canvas")));
		driver.click(itemNamed(scene.root.canvasMenu, "Clear canvas"));

		CHECK(!hasPaintedPixels(scene.document));
	}
}

SUITE(Sketch2DBrushPressure) {
	TEST(BrushPopupTogglesPressurePerParameter) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;

		driver.click(scene.root.toolBar.brushSize);
		REQUIRE CHECK(driver.ui().isPopupOpen(popup));
		CHECK(!popup.sizePressure.isChecked());

		driver.click(popup.pressureSection.header());
		driver.click(popup.sizePressure);
		CHECK(scene.brush.usesPressure(PressureTarget::Size));
		CHECK(!scene.brush.usesPressure(PressureTarget::Opacity));

		driver.click(popup.flowPressure);
		CHECK(scene.brush.usesPressure(PressureTarget::Flow));
	}

	TEST(MouseStrokeIgnoresPressureCurve) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setUsesPressure(PressureTarget::Size, true);
		scene.brush.set(BrushParameter::MinSize, 0);
		scene.brush.setSize(40);

		const RectF canvas = scene.root.canvas.windowRect();
		driver.drag(canvas.center(), canvas.center() + Vec2F(60, 0));
		CHECK(hasPaintedPixels(scene.document));
	}
}

SUITE(Sketch2DBrushPopupSections) {
	TEST(BasicSlidersAreAlwaysVisibleAndGroupsStartClosed) {
		Sketch2DScene scene;
		scene.brush.apply(*findBrushPreset(BrushType::Calligraphy));
		UiDriver driver(scene.root, { 1280, 800 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		popup.refresh();
		driver.click(scene.root.toolBar.brushSize);

		for (Widget* basic :
				 std::initializer_list<Widget*>{ &popup.size, &popup.opacity, &popup.flow, &popup.hardness, &popup.stabilizer })
			CHECK(basic->parent() == &popup);
		for (Collapsible* section : { &popup.shapeSection, &popup.textureSection, &popup.pressureSection }) {
			CHECK(!section->isExpanded());
			CHECK(!section->isBodyShown());
		}
		CHECK_EQUAL("25%  45°", popup.shapeSection.header().summary());
		CHECK_EQUAL("size", popup.pressureSection.header().summary());
	}

	TEST(OpenedGroupGrowsThePopupAndFillsItsWidth) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		driver.click(scene.root.toolBar.brushSize);
		const halnf closedHeight = popup.rect().size.y;

		driver.click(popup.shapeSection.header());
		CHECK(popup.shapeSection.isExpanded());
		CHECK(popup.rect().size.y > closedHeight + 60);
		const RectF body = popup.shapeSection.body().windowRect();
		const RectF slider = popup.angle.windowRect();
		CHECK_CLOSE(body.pos.x + body.size.x, slider.pos.x + slider.size.x, 0.5f);
	}

	TEST(SummaryFollowsGroupedSliderChanges) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		driver.click(scene.root.toolBar.brushSize);
		driver.click(popup.textureSection.header());
		driver.setSlider(popup.texture.slider(), 0.6f);
		CHECK_EQUAL(percentLabel(scene.brush.get(BrushParameter::Texture)), popup.textureSection.header().summary());
	}
}

SUITE(Sketch2DBrushShape) {
	TEST(BrushPopupEditsRoundnessAngleAndSpacing) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 1200 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		driver.click(scene.root.toolBar.brushSize);
		REQUIRE CHECK(driver.ui().isPopupOpen(popup));

		driver.click(popup.shapeSection.header());
		driver.click(popup.textureSection.header());
		driver.setSlider(popup.roundness.slider(), 0.5f);
		driver.setSlider(popup.angle.slider(), 90);
		driver.setSlider(popup.spacing.slider(), 1);
		driver.setSlider(popup.texture.slider(), 0.5f);

		CHECK_CLOSE(0.5f, scene.brush.get(BrushParameter::Roundness), 0.03f);
		CHECK_CLOSE(90, scene.brush.get(BrushParameter::Angle), 3);
		CHECK_CLOSE(1, scene.brush.spacing(), 0.03f);
		CHECK_CLOSE(0.5f, scene.brush.get(BrushParameter::Texture), 0.03f);
	}

	TEST(SmudgeGroupAppearsOnlyForTheSmudgeBrush) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 1200 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		driver.click(scene.root.toolBar.brushSize);
		CHECK(!popup.smudgeSection.isVisible());
		driver.click(scene.root.toolBar.brushSize);

		driver.click(scene.root.toolStrip.button("smudge"));
		driver.click(scene.root.toolBar.brushSize);
		REQUIRE CHECK(popup.smudgeSection.isVisible());
		CHECK(!popup.markerBlend.isVisible());

		driver.click(popup.smudgeSection.header());
		driver.setSlider(popup.blur.slider(), 4);
		driver.setSlider(popup.smudgeAmount.slider(), 0.5f);
		driver.click(popup.linearMixing);
		CHECK_CLOSE(4, scene.brush.get(BrushParameter::Blur), 0.1f);
		CHECK_CLOSE(0.5f, scene.brush.get(BrushParameter::Smudge), 0.02f);
		CHECK(scene.brush.linearMixing());
		CHECK(popup.smudgeSection.header().summary().ends_with("linear"));
	}

	TEST(ShapeGroupTogglesMarkerBlending) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 1200 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		driver.click(scene.root.toolBar.brushSize);
		driver.click(popup.shapeSection.header());
		CHECK(!popup.markerBlend.isChecked());

		driver.click(popup.markerBlend);
		CHECK(scene.brush.markerBlend());
		CHECK(popup.shapeSection.header().summary().ends_with("marker"));

		scene.brush.apply(*findBrushPreset(BrushType::Round));
		popup.refresh();
		CHECK(!popup.markerBlend.isChecked());
	}
}

SUITE(Sketch2DBrushList) {
	TEST(BrushButtonOpensPresetListAndAppliesChoice) {
		Sketch2DScene scene;
		scene.brush.apply(*findBrushPreset(BrushType::Round));
		UiDriver driver(scene.root, { 1280, 800 });
		BrushListPopup& list = scene.root.toolBar.brushListPopup;

		driver.click(scene.root.toolBar.brushTool);
		REQUIRE CHECK(driver.ui().isPopupOpen(list));
		CHECK_EQUAL(9u, list.buttons().size());
		CHECK(list.button("Round brush").isSelected());

		driver.click(list.button("Calligraphy"));
		CHECK(scene.brush.type == BrushType::Calligraphy);
		CHECK_EQUAL("Calligraphy", scene.root.toolBar.brushTool.text());
		CHECK_EQUAL("12 px", scene.root.toolBar.brushSize.text());
		CHECK_CLOSE(45, scene.root.toolBar.brushPopup.angle.value(), 0.01f);
		CHECK(list.button("Calligraphy").isSelected());
		CHECK(!list.button("Round brush").isSelected());
	}

	TEST(PresetRowsShowAStrokeSampleWithTheNameOnTheLeft) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		BrushListPopup& list = scene.root.toolBar.brushListPopup;
		driver.click(scene.root.toolBar.brushTool);
		REQUIRE CHECK(driver.ui().isPopupOpen(list));
		driver.settle();

		for (const auto& button : list.buttons()) {
			const RectF row = button->windowRect();
			bool sampled = false;
			bool leftText = false;
			for (const auto& command : driver.paint().commands()) {
				const RectF& rect = command.rect;
				if (command.text == button->text()) leftText = rect.pos.x < row.pos.x + row.size.x * 0.35f;
				else if (
					command.text.empty() && rect.pos.y >= row.pos.y && rect.pos.y + rect.size.y <= row.pos.y + row.size.y &&
					rect.pos.x >= row.pos.x && rect.pos.x + rect.size.x <= row.pos.x + row.size.y * 1.5f &&
					rect.size.x < row.size.x * 0.5f
				)
					sampled = true;
			}
			CHECK(leftText);
			CHECK(sampled);
		}
	}

	TEST(EraserRemembersThePreviousBrush) {
		Sketch2DScene scene;
		scene.brush.apply(*findBrushPreset(BrushType::Ink));
		UiDriver driver(scene.root, { 1280, 800 });

		driver.click(scene.root.toolStrip.button("eraser"));
		CHECK(scene.brush.type == BrushType::Eraser);
		CHECK_EQUAL(32.0f, scene.brush.size());
		CHECK_CLOSE(32, scene.root.toolBar.brushPopup.size.value(), 0.01f);

		driver.click(scene.root.toolStrip.button("brush"));
		CHECK(scene.brush.type == BrushType::Ink);
		CHECK_EQUAL(7.0f, scene.brush.size());
	}
}

SUITE(Sketch2DStabilizer) {
	halnf strokeDeviation(Document & document, halnf fromX, halnf toX, halnf centerY) {
		document.settlePaint();
		halnf sum = 0;
		int count = 0;
		for (int4 x = (int4) fromX; x < (int4) toX; x += 4) {
			halnf weighted = 0;
			halnf total = 0;
			for (int4 y = (int4) centerY - 40; y < (int4) centerY + 40; y++) {
				const Tile* tile = document.activeLayer()->tileIfExists({ floordiv(x, kTileSize), floordiv(y, kTileSize) });
				if (!tile) continue;
				const int4 lx = x - floordiv(x, kTileSize) * kTileSize;
				const int4 ly = y - floordiv(y, kTileSize) * kTileSize;
				const halnf alpha = tile->pixels()[((ualni) ly * kTileSize + lx) * 4 + 3];
				weighted += alpha * (halnf) y;
				total += alpha;
			}
			if (total <= 0) continue;
			const halnf offset = weighted / total - centerY;
			sum += offset * offset;
			count++;
		}
		return count ? std::sqrt(sum / (halnf) count) : 0;
	}

	halnf drawJitteryLine(Sketch2DScene & scene, UiDriver & driver, halnf amount) {
		scene.brush.setStabilizer(amount);
		scene.brush.setSize(6);
		scene.brush.setHardness(1);
		const RectF canvas = scene.root.canvas.windowRect();
		const Vec2F start = canvas.pos + Vec2F(100, 300);
		driver.moveTo(start);
		driver.press();
		for (int step = 1; step < 160; step++)
			driver.moveTo(start + Vec2F((halnf) step * 3.0f, (step / 3) % 2 ? 10.0f : -10.0f));
		driver.release();
		const Vec2F world = scene.document.screenToWorld(scene.root.canvas.toLocal(start));
		return strokeDeviation(scene.document, world.x + 80, world.x + 400, world.y);
	}

	TEST(StabilizedStrokeIsStraighterThanRawInput) {
		Sketch2DScene raw;
		UiDriver rawDriver(raw.root, { 1280, 800 });
		Sketch2DScene smooth;
		UiDriver smoothDriver(smooth.root, { 1280, 800 });

		const halnf rawDeviation = drawJitteryLine(raw, rawDriver, 0);
		const halnf smoothDeviation = drawJitteryLine(smooth, smoothDriver, 0.6f);
		CHECK(rawDeviation > 3.0f);
		CHECK(smoothDeviation < rawDeviation * 0.4f);
	}

	TEST(StabilizerSliderAndMenuShareOneValue) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 1200 });
		CHECK_CLOSE(0.325f, scene.brush.stabilizer(), 1e-6f);

		scene.root.commands.run(Command::StabilizerIncrease);
		CHECK_CLOSE(0.425f, scene.brush.stabilizer(), 1e-5f);
		CHECK_CLOSE(0.425f, scene.root.toolBar.brushPopup.stabilizer.value(), 1e-5f);
		CHECK_EQUAL("Stabilizer 43%", scene.root.lastNotice());

		for (int index = 0; index < 10; index++)
			scene.root.commands.run(Command::StabilizerDecrease);
		CHECK_EQUAL(0.0f, scene.brush.stabilizer());
	}

	TEST(GlobalStabilizerIgnoresTheBrushValue) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setStabilizer(0.9f);
		ui::MenuItem* toggle = nullptr;
		for (const auto& item : scene.root.stabilizerMenu.items()) {
			if (item->entry().text == "Use brush stabilizer") toggle = item.get();
		}
		REQUIRE CHECK(toggle != nullptr);
		toggle->activate();
		CHECK(!scene.root.canvas.settings().useBrushStabilizer);
		CHECK_CLOSE(Brush::kDefaultStabilizer, scene.root.canvas.activeStabilizer(), 1e-6f);

		scene.root.commands.run(Command::StabilizerIncrease);
		CHECK_CLOSE(0.9f, scene.brush.stabilizer(), 1e-6f);
		CHECK_CLOSE(0.425f, scene.root.canvas.activeStabilizer(), 1e-5f);
	}

	TEST(PresetsKeepTheirOwnStabilizerUntilReset) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setStabilizer(0.8f);
		for (const auto& item : scene.root.stabilizerMenu.items()) {
			if (item->entry().text == "Reset all brush stabilization") item->activate();
		}
		CHECK_CLOSE(Brush::kDefaultStabilizer, scene.brush.stabilizer(), 1e-6f);
	}

	TEST(PenRestingLetsTheStabilizedLineReachIt) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setStabilizer(0.8f);
		scene.root.canvas.settings().line.holdForLine = false;
		const RectF canvas = scene.root.canvas.windowRect();
		driver.moveTo(canvas.pos + Vec2F(100, 300));
		driver.press();
		driver.moveTo(canvas.pos + Vec2F(400, 300));
		const halnf behind = scene.root.canvas.stabilizer().smoothed().position.x;
		CHECK(scene.root.canvas.isStabilizerRopeVisible());
		CHECK(behind < canvas.pos.x + 400 - scene.root.canvas.windowRect().pos.x - 20);

		driver.advance(1500);
		CHECK(scene.root.canvas.stabilizer().isSettled());
		CHECK(!scene.root.canvas.isStabilizerRopeVisible());
		driver.release();
	}
}

SUITE(Sketch2DBrushPopupLayout) {
	TEST(BasicSlidersComeFirstThenGroupsWithoutOverlap) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 900 });
		BrushPopup& popup = scene.root.toolBar.brushPopup;
		driver.click(scene.root.toolBar.brushSize);
		driver.click(popup.shapeSection.header());

		const std::vector<Widget*> order = { &popup.size,           &popup.opacity,        &popup.flow,
																				 &popup.hardness,       &popup.stabilizer,     &popup.shapeSection,
																				 &popup.textureSection, &popup.pressureSection };
		for (size_t index = 1; index < order.size(); index++) {
			const RectF above = order[index - 1]->windowRect();
			CHECK(order[index]->windowRect().pos.y >= above.pos.y + above.size.y - 0.5f);
		}
		const RectF panel = popup.windowRect();
		const RectF last = popup.pressureSection.windowRect();
		CHECK(last.pos.y + last.size.y <= panel.pos.y + panel.size.y + 0.5f);
		CHECK(driver.paint().isBalanced());
	}
}

SUITE(Sketch2DSessionRecording) {
	struct RecordingScene : Sketch2DScene {
		FakeRecorderHost host;
		SessionRecorder recorder{ host };
		SessionRecordingControl recording{ recorder };
		std::filesystem::path directory = std::filesystem::temp_directory_path() / "sketch2d-ui-sessions";

		RecordingScene() {
			std::filesystem::remove_all(directory);
			setenv("MODULES_SESSION_DIR", directory.c_str(), 1);
			root.setSessionRecording(&recording);
			recording.setTimestamp([]() { return std::string("take-1"); });
		}

		~RecordingScene() {
			root.setSessionRecording(nullptr);
			unsetenv("MODULES_SESSION_DIR");
			std::filesystem::remove_all(directory);
		}

		void runDebugItem(UiDriver& driver, const std::string& item) {
			driver.click(menuTitle(root, menuIndex(root, "Debug")));
			driver.click(itemNamed(root.debugMenu, item));
		}

		std::string stopAndSave(UiDriver& driver) {
			root.commands.run(Command::StopSessionRecording);
			driver.settle();
			if (!recording.saveDialog.isOpen()) return {};
			driver.key(InputID::ENTER);
			driver.settle();
			return recording.lastSessionDirectory();
		}
	};

	TEST(StopAsksWhereToSaveAndSavesTheChosenFolder) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.apply(*findBrushPreset(BrushType::Ink));
		scene.document.setZoom(2);
		CHECK(!scene.recording.badge.isVisible());

		scene.runDebugItem(driver, "Record session");
		REQUIRE CHECK(scene.root.isRecordingSession());
		CHECK(scene.host.tap != nullptr);
		CHECK(scene.recording.badge.isVisible());

		const RectF canvas = scene.root.canvas.windowRect();
		scene.host.tap->onPointerMoved(canvas.center());
		scene.host.tap->onPointerButton(InputID::MOUSE1, true);
		scene.host.clockMs += 65000;
		scene.host.tap->onPointerButton(InputID::MOUSE1, false);
		driver.advance(32);
		CHECK(scene.recording.badge.text().ends_with("REC 1:05"));

		scene.runDebugItem(driver, "Stop recording and save");
		driver.settle();
		CHECK(!scene.root.isRecordingSession());
		CHECK(scene.host.tap == nullptr);
		CHECK(!scene.recording.badge.isVisible());
		REQUIRE CHECK(scene.recording.saveDialog.isOpen());
		CHECK(scene.recording.hasUnsavedSession());
		CHECK(scene.recording.saveDialog.mode() == ui::FileDialog::Mode::Save);
		CHECK_EQUAL(scene.directory.string(), scene.recording.saveDialog.browser.directory());
		CHECK_EQUAL("take-1", scene.recording.saveDialog.name.text());
		CHECK(scene.recording.saveDialog.message().find("65.0 s") != std::string::npos);

		scene.recording.saveDialog.name.setText("fast-ink");
		driver.key(InputID::ENTER);
		const std::filesystem::path saved = scene.directory / "fast-ink";
		CHECK_EQUAL(saved.string(), scene.recording.lastSessionDirectory());
		CHECK(!scene.recording.hasUnsavedSession());
		CHECK(scene.root.lastNotice().find("fast-ink") != std::string::npos);

		const std::optional<RecordedSession> session = RecordedSession::load(saved.string());
		REQUIRE CHECK(session.has_value());
		REQUIRE CHECK_EQUAL(3u, session->input.events.size());
		CHECK_EQUAL(65000.0, session->input.events.back().timeMs);
		CHECK(session->snapshot.brush.type == BrushType::Ink);
		CHECK_EQUAL(2.0f, session->snapshot.view.zoom);
		CHECK_EQUAL("fake gpu", session->snapshot.environment.device);
		CHECK(session->recordedFrames.rfind("frame\twall_ms\tcpu_ms", 0) == 0);
	}

	TEST(NextSaveStartsInTheFolderUsedLastTime) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const std::filesystem::path elsewhere = scene.directory / "nested";
		std::filesystem::create_directories(elsewhere);

		scene.root.commands.run(Command::RecordSession);
		scene.root.commands.run(Command::StopSessionRecording);
		driver.settle();
		scene.recording.saveDialog.name.setText((elsewhere / "first").string());
		driver.key(InputID::ENTER);
		CHECK_EQUAL((elsewhere / "first").string(), scene.recording.lastSessionDirectory());

		scene.root.commands.run(Command::RecordSession);
		scene.root.commands.run(Command::StopSessionRecording);
		driver.settle();
		CHECK_EQUAL(elsewhere.string(), scene.recording.saveDialog.browser.directory());
	}

	TEST(CancellingTheSaveAsksBeforeDiscarding) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::RecordSession);
		scene.root.commands.run(Command::StopSessionRecording);
		driver.settle();

		driver.click(*scene.recording.saveDialog.buttons().back());
		driver.settle();
		REQUIRE CHECK(scene.recording.discardDialog.isOpen());
		CHECK_EQUAL("Discard the recording?", scene.recording.discardDialog.title());
		CHECK(scene.recording.hasUnsavedSession());

		driver.click(*scene.recording.discardDialog.buttons().back());
		driver.settle();
		CHECK(scene.recording.saveDialog.isOpen());
		CHECK(scene.recording.hasUnsavedSession());

		driver.click(*scene.recording.saveDialog.buttons().back());
		driver.settle();
		driver.click(*scene.recording.discardDialog.buttons().front());
		CHECK(!scene.recording.hasUnsavedSession());
		CHECK_EQUAL("Recording discarded", scene.root.lastNotice());
		CHECK(!std::filesystem::exists(scene.directory / "take-1"));
	}

	TEST(StopIsDisabledUntilRecordingAndRecordWhileRecording) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		CHECK(!scene.root.commands.isEnabled(Command::StopSessionRecording));
		CHECK(scene.root.commands.isEnabled(Command::RecordSession));
		scene.root.commands.run(Command::RecordSession);
		CHECK(!scene.root.commands.isEnabled(Command::RecordSession));
		CHECK(scene.root.commands.isEnabled(Command::StopSessionRecording));
	}

	TEST(RecordButtonIsAWideHoverableButtonThatStops) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::RecordSession);
		driver.settle();
		Button& button = scene.recording.badge;
		CHECK(driver.paint().hasText(button.text()));
		CHECK(button.text().ends_with("REC 0:00"));
		CHECK(button.windowRect().size.x >= 100);
		CHECK_CLOSE(scene.root.toolBar.undo.windowRect().size.y, button.windowRect().size.y, 1.0f);

		driver.moveTo(button);
		driver.settle();
		CHECK(button.isHovered());
		CHECK_EQUAL("Recording session. Click to stop and save", button.tooltip());

		driver.click(button);
		driver.settle();
		CHECK(!scene.root.isRecordingSession());
		CHECK(scene.recording.saveDialog.isOpen());
	}

	TEST(RecordingWithoutARecorderIsUnavailable) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		CHECK(!scene.root.commands.isEnabled(Command::RecordSession));
		CHECK(!scene.root.commands.run(Command::RecordSession));
	}

	TEST(RecordingCapturesTheStateBeforeTheFirstStroke) {
		RecordingScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const RectF canvas = scene.root.canvas.windowRect();
		driver.drag(canvas.center(), canvas.center() + Vec2F(80.0f, 0.0f));
		const ualni tilesBefore = scene.document.tileCount();

		scene.root.commands.run(Command::RecordSession);
		driver.drag(canvas.center() + Vec2F(0.0f, 300.0f), canvas.center() + Vec2F(80.0f, 300.0f));
		const std::string savedTo = scene.stopAndSave(driver);

		const std::optional<RecordedSession> saved = RecordedSession::load(savedTo);
		REQUIRE CHECK(saved.has_value());
		Document restored;
		Brush brush;
		ViewSettings settings;
		saved->snapshot.applyTo(restored, brush, settings);
		CHECK_EQUAL(tilesBefore, restored.tileCount());
		CHECK(scene.document.tileCount() > tilesBefore);
	}
}

SUITE(Sketch2DTouchGestures) {
	TEST(TwoFingerDragPansTheCanvas) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F center = scene.root.canvas.windowRect().center();
		const Vec2F before = scene.document.screenToWorld(scene.root.canvas.toLocal(center));

		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, center + Vec2F(40, 30), { 40, 30 } }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, center + Vec2F(40, 30) }));

		const Vec2F after = scene.document.screenToWorld(scene.root.canvas.toLocal(center + Vec2F(40, 30)));
		CHECK_CLOSE(before.x, after.x, 0.01f);
		CHECK_CLOSE(before.y, after.y, 0.01f);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(PinchZoomsAroundTheFingers) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F point = scene.root.canvas.windowRect().pos + Vec2F(200, 150);
		const Vec2F anchor = scene.document.screenToWorld(scene.root.canvas.toLocal(point));
		const halnf zoom = scene.document.zoom();

		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, point }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, point, { 0, 0 }, 2.0f }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, point }));

		CHECK_CLOSE(zoom * 2.0f, scene.document.zoom(), 0.001f);
		const Vec2F stays = scene.document.screenToWorld(scene.root.canvas.toLocal(point));
		CHECK_CLOSE(anchor.x, stays.x, 0.01f);
		CHECK_CLOSE(anchor.y, stays.y, 0.01f);
	}

	TEST(SecondFingerDiscardsTheDabTheFirstFingerStarted) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F center = scene.root.canvas.windowRect().center();

		driver.moveTo(center);
		driver.press();
		driver.moveTo(center + Vec2F(4, 0));
		REQUIRE CHECK(scene.root.canvas.isPainting());

		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center + Vec2F(50, 0) }));
		driver.release();
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, center + Vec2F(60, 0), { 10, 0 } }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, center + Vec2F(60, 0) }));

		CHECK(!scene.root.canvas.isPainting());
		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(FingerThatBecomesAPinchLeavesNoDab) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setAutoSettle(false);
		driver.setPointerTool(PointerTool::Finger);
		const Vec2F center = scene.root.canvas.windowRect().center();
		driver.moveTo(center);
		driver.press();
		driver.advance(30);
		CHECK(scene.root.canvas.isWaitingForFinger());
		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center + Vec2F(60, 0) }));
		driver.release();
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, center + Vec2F(70, 0), { 10, 0 }, 1.3f }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, center + Vec2F(70, 0) }));
		driver.advance(400);
		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(LastFingerLiftedAfterAPinchDoesNotPaint) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setAutoSettle(false);
		driver.setPointerTool(PointerTool::Finger);
		const Vec2F center = scene.root.canvas.windowRect().center();
		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, center, { 0, 0 }, 0.8f }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, center }));
		driver.moveTo(center + Vec2F(30, 10));
		driver.press();
		driver.advance(120);
		driver.moveTo(center + Vec2F(80, 10));
		driver.release();
		driver.advance(400);
		CHECK_EQUAL(0u, scene.document.tileCount());
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(SingleFingerHoldStartsPaintingAfterAShortWait) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setAutoSettle(false);
		driver.setPointerTool(PointerTool::Finger);
		const Vec2F center = scene.root.canvas.windowRect().center();
		driver.moveTo(center);
		driver.press();
		CHECK(!scene.root.canvas.isPainting());
		CHECK_EQUAL(0u, scene.document.tileCount());
		driver.advance(CanvasWidget::kFingerCommitMs + 40);
		CHECK(scene.root.canvas.isPainting());
		driver.moveTo(center + Vec2F(5, 0));
		driver.release();
		CHECK(scene.document.tileCount() > 0);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
	}

	TEST(QuickFingerStrokeKeepsEveryPointItMoved) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setAutoSettle(false);
		driver.setPointerTool(PointerTool::Finger);
		const Vec2F start = scene.root.canvas.windowRect().center();
		driver.moveTo(start);
		driver.press();
		driver.moveTo(start + Vec2F(6, 0));
		driver.moveTo(start + Vec2F(12, 0));
		CHECK(scene.root.canvas.isWaitingForFinger());
		driver.moveTo(start + Vec2F(200, 0));
		CHECK(scene.root.canvas.isPainting());
		driver.release();
		driver.advance(500);
		const Vec2F a = scene.document.screenToWorld(scene.root.canvas.toLocal(start + Vec2F(3, 0)));
		const Vec2F b = scene.document.screenToWorld(scene.root.canvas.toLocal(start + Vec2F(170, 0)));
		CHECK(scene.document.sampleColor(a, false).a > 0.5f);
		CHECK(scene.document.sampleColor(b, false).a > 0.5f);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
	}

	TEST(FingerTapStillLeavesADot) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setAutoSettle(false);
		driver.setPointerTool(PointerTool::Finger);
		const Vec2F center = scene.root.canvas.windowRect().center();
		driver.moveTo(center);
		driver.press();
		driver.release();
		CHECK(scene.document.tileCount() > 0);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
	}

	TEST(PenIsNeverDelayed) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.setAutoSettle(false);
		driver.setPointerTool(PointerTool::Pen);
		const Vec2F center = scene.root.canvas.windowRect().center();
		driver.moveTo(center);
		driver.press();
		CHECK(scene.root.canvas.isPainting());
		CHECK(scene.document.tileCount() > 0);
		driver.release();
	}

	TEST(OneFingerStillPaints) {
		Sketch2DScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F center = scene.root.canvas.windowRect().center();

		driver.drag(center, center + Vec2F(80, 0));

		CHECK(scene.document.tileCount() > 0);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
	}
}
