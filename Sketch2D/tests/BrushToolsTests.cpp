#include "TestSketchesRoot.hpp"
#include "FakeWindowHost.hpp"
#include "Sketch2DUI.hpp"
#include "StudioUI.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class ToolsViewport : public ViewportRenderer {
	public:
		void render(Document&, const ViewSettings&, Vec2F) override {}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }
	};

	class ToolsIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string& name) const override { return { (ualni) name.size() + 1 }; }
	};

	struct ToolsScene {
		Document document;
		Brush brush;
		ToolsViewport viewport;
		ToolsIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };

		ToolsScene() {
			root.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
		}
	};

	struct TabletScene {
		Document document;
		Brush brush;
		ToolsViewport viewport;
		ToolsIcons icons;
		Sketch2DRoot classic{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };
		StudioRoot studio{ classic, brush, icons };

		TabletScene() {
			classic.canvas.settings().viewStiffness = 1;
			brush.apply(*findBrushPreset(BrushType::Round));
		}
	};

	const MenuItem* findItem(Menu& menu, const std::string& text) {
		for (const auto& item : menu.items())
			if (item->text() == text) return item.get();
		return nullptr;
	}

	void choose(Menu& menu, const std::string& text) {
		const MenuItem* item = findItem(menu, text);
		CHECK(item != nullptr);
		if (item) item->entry().action();
	}

	size_t strokesOf(Document& document) { return document.history().revision(); }
}

SUITE(BrushReset) {
	TEST(ResetEverythingRestoresThePresetAndKeepsColor) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Ink));
		const Brush original = brush;
		brush.color = { 0.1f, 0.2f, 0.3f, 1 };
		brush.setSize(321);
		brush.set(BrushParameter::Texture, 0.9f);
		brush.setStabilizer(0.9f);
		brush.resetEverything();
		CHECK_CLOSE(original.size(), brush.size(), 1e-4f);
		CHECK_CLOSE(original.get(BrushParameter::Texture), brush.get(BrushParameter::Texture), 1e-4f);
		CHECK_CLOSE(Brush::kDefaultStabilizer, brush.stabilizer(), 1e-4f);
		CHECK_CLOSE(0.1f, brush.color.r, 1e-6f);
		CHECK_EQUAL(std::string(original.presetName()), brush.presetName());
	}

	TEST(ResetParametersKeepsTexturesAndStabilization) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Pencil));
		const Brush original = brush;
		brush.setSize(555);
		brush.setOpacity(0.2f);
		brush.set(BrushParameter::Texture, 0.77f);
		brush.set(BrushParameter::GrainScale, 4.5f);
		brush.setStabilizer(0.8f);
		brush.resetParameters();
		CHECK_CLOSE(original.size(), brush.size(), 1e-4f);
		CHECK_CLOSE(original.opacity(), brush.opacity(), 1e-4f);
		CHECK_CLOSE(0.77f, brush.get(BrushParameter::Texture), 1e-4f);
		CHECK_CLOSE(4.5f, brush.get(BrushParameter::GrainScale), 1e-4f);
		CHECK_CLOSE(0.8f, brush.stabilizer(), 1e-4f);
	}

	TEST(ResetTexturesOnlyTouchesTextureValues) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Pencil));
		const Brush original = brush;
		brush.setSize(555);
		brush.set(BrushParameter::Texture, 0.11f);
		brush.set(BrushParameter::DeckleScale, 9);
		brush.resetTextures();
		CHECK_CLOSE(555.0f, brush.size(), 1e-4f);
		CHECK_CLOSE(original.get(BrushParameter::Texture), brush.get(BrushParameter::Texture), 1e-4f);
		CHECK_CLOSE(original.get(BrushParameter::DeckleScale), brush.get(BrushParameter::DeckleScale), 1e-4f);
	}

	TEST(ResetStabilizationOnlyTouchesTheStabilizer) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Round));
		brush.setSize(77);
		brush.setStabilizer(0.9f);
		brush.resetStabilization();
		CHECK_CLOSE(Brush::kDefaultStabilizer, brush.stabilizer(), 1e-4f);
		CHECK_CLOSE(77.0f, brush.size(), 1e-4f);
	}

	TEST(SizeStepsAlwaysMoveAndStayWithinLimits) {
		Brush brush;
		brush.setSize(1);
		brush.scaleSize(1.2f);
		CHECK_CLOSE(2.0f, brush.size(), 1e-4f);
		brush.setSize(100);
		brush.scaleSize(1.2f);
		CHECK_CLOSE(120.0f, brush.size(), 1e-4f);
		brush.scaleSize(1.0f / 1.2f);
		CHECK_CLOSE(100.0f, brush.size(), 1e-4f);
		brush.setSize(1);
		brush.scaleSize(1.0f / 1.2f);
		CHECK_CLOSE(1.0f, brush.size(), 1e-4f);
		brush.setSize(Brush::kMaximumSize);
		brush.scaleSize(1.2f);
		CHECK_CLOSE(Brush::kMaximumSize, brush.size(), 1e-4f);
	}

	TEST(AngleWrapsAroundAFullTurn) {
		Brush brush;
		brush.set(BrushParameter::Angle, 350);
		brush.adjust(BrushParameter::Angle, 15);
		CHECK_CLOSE(5.0f, brush.get(BrushParameter::Angle), 1e-3f);
		brush.adjust(BrushParameter::Angle, -15);
		CHECK_CLOSE(350.0f, brush.get(BrushParameter::Angle), 1e-3f);
	}
}

SUITE(BrushCommands) {
	TEST(CommandIdsMatchTheReference) {
		CHECK_EQUAL(1000178, (int4) Command::ToolBrushAngle);
		CHECK_EQUAL(1000179, (int4) Command::ToolBrushSize);
		CHECK_EQUAL(1000183, (int4) Command::HoldBrushFlow);
		CHECK_EQUAL(1000185, (int4) Command::HoldBrushOpacity);
		CHECK_EQUAL(1000202, (int4) Command::ResetBrush);
		CHECK_EQUAL(1000206, (int4) Command::BrushAngleDecrease);
		CHECK_EQUAL(1000209, (int4) Command::BrushFlowIncrease);
		CHECK_EQUAL(1000211, (int4) Command::BrushOpacityIncrease);
		CHECK_EQUAL(std::string("Reset brush"), commandInfo(Command::ResetBrush).label);
		CHECK_EQUAL(std::string("Brush flow tool"), commandInfo(Command::HoldBrushFlow).label);
		CHECK_EQUAL(std::string("Opacity increase"), commandInfo(Command::BrushOpacityIncrease).label);
	}

	TEST(BracketKeysResizeTheBrush) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setSize(100);
		driver.key(InputID::RIGHT_BRACKET);
		CHECK_CLOSE(115.0f, scene.brush.size(), 1e-3f);
		driver.holdKey(InputID::LEFT_BRACKET, 2);
		CHECK_CLOSE(71.0f, scene.brush.size(), 1e-3f);
		CHECK_EQUAL(std::string("71 px"), scene.root.lastNotice());
		CHECK_EQUAL(std::string("71 px"), scene.root.toolBar.brushSize.text());
	}

	TEST(DigitKeysSetBrushOpacity) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.key(InputID::N3);
		CHECK_CLOSE(0.3f, scene.brush.opacity(), 1e-4f);
		driver.key(InputID::N0);
		CHECK_CLOSE(1.0f, scene.brush.opacity(), 1e-4f);
		driver.key(InputID::N1);
		CHECK_CLOSE(0.1f, scene.brush.opacity(), 1e-4f);
	}

	TEST(StepCommandsAdjustFlowOpacityAndAngle) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setFlow(0.5f);
		scene.brush.setOpacity(0.5f);
		scene.root.commands.run(Command::BrushFlowIncrease);
		scene.root.commands.run(Command::BrushOpacityDecrease);
		scene.root.commands.run(Command::BrushAngleIncrease);
		CHECK_CLOSE(0.55f, scene.brush.flow(), 1e-4f);
		CHECK_CLOSE(0.45f, scene.brush.opacity(), 1e-4f);
		CHECK_CLOSE(kBrushAngleStep, scene.brush.get(BrushParameter::Angle), 1e-4f);
	}

	TEST(BrushMenuListsTheBrushCommands) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		Menu& menu = scene.root.brushMenu;
		for (const char* text : { "Reset all brushes",
															"Smaller brush",
															"Bigger brush",
															"Reset brush",
															"Show Brush list",
															"Show Brush options",
															"Adjust",
															"Opacity",
															"Stabilizer" })
			CHECK(findItem(menu, text) != nullptr);
		for (const char* text : { "Reset all brushes (everything)",
															"Reset all brush parameters",
															"Reset all brush stabilization",
															"Reset all brush textures" })
			CHECK(findItem(scene.root.resetMenu, text) != nullptr);
		CHECK_EQUAL((size_t) 10, scene.root.brushOpacityMenu.items().size());
		CHECK_EQUAL(std::string("]"), findItem(menu, "Bigger brush")->entry().shortcut);
	}

	TEST(ResetMenuResetsTheCurrentBrush) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const halnf original = scene.brush.size();
		scene.brush.setSize(999);
		scene.brush.setStabilizer(0.95f);
		choose(scene.root.resetMenu, "Reset all brush parameters");
		CHECK_CLOSE(original, scene.brush.size(), 1e-4f);
		CHECK_CLOSE(0.95f, scene.brush.stabilizer(), 1e-4f);
		choose(scene.root.resetMenu, "Reset all brush stabilization");
		CHECK_CLOSE(Brush::kDefaultStabilizer, scene.brush.stabilizer(), 1e-4f);
	}

	TEST(ResetAllBrushesRestoresThePaintingBrush) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.apply(*findBrushPreset(BrushType::Ink));
		scene.brush.setSize(800);
		driver.click(scene.root.toolStrip.button("eraser"));
		scene.root.commands.run(Command::ResetAllBrushes);
		driver.click(scene.root.toolStrip.button("brush"));
		CHECK(scene.brush.type == BrushType::Round);
		CHECK(scene.brush.size() < 800);
	}

	TEST(OptionsPanelResetButtonResetsTheBrush) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const halnf original = scene.brush.size();
		scene.brush.setSize(444);
		scene.root.refreshFromState();
		driver.layout();
		driver.click(scene.root.toolBar.brushSize);
		REQUIRE CHECK(driver.ui().isPopupOpen(scene.root.toolBar.brushPopup));
		driver.click(scene.root.toolBar.brushPopup.reset);
		CHECK_CLOSE(original, scene.brush.size(), 1e-4f);
		CHECK_CLOSE(original, scene.root.toolBar.brushPopup.size.value(), 0.5f);
	}

	TEST(PanelShortcutsOpenTheBrushPopups) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		driver.moveTo(driver.centerOf(scene.root.canvas));
		driver.key(InputID::F4);
		CHECK(driver.ui().isPopupOpen(scene.root.toolBar.brushListPopup));
		driver.key(InputID::ESCAPE);
		driver.key(InputID::F5);
		CHECK(driver.ui().isPopupOpen(scene.root.toolBar.brushPopup));
	}
}

SUITE(BrushDragTools) {
	TEST(DragValuesFollowTheReferenceFeel) {
		CHECK_CLOSE(
			200.0f, CanvasWidget::adjustedValue(BrushParameter::Size, 100, CanvasWidget::kSizeDoublingPixels), 0.5f
		);
		CHECK_CLOSE(
			50.0f, CanvasWidget::adjustedValue(BrushParameter::Size, 100, -CanvasWidget::kSizeDoublingPixels), 0.5f
		);
		CHECK_CLOSE(10.0f, CanvasWidget::adjustedValue(BrushParameter::Angle, 350, 20), 1e-3f);
		CHECK_CLOSE(0.75f, CanvasWidget::adjustedValue(BrushParameter::Opacity, 0.5f, 50), 1e-4f);
	}

	TEST(HoldingDAndDraggingResizesWithoutPainting) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setSize(50);
		const Vec2F center = driver.centerOf(scene.root.canvas);
		driver.moveTo(center);
		const size_t before = strokesOf(scene.document);
		driver.pressKey(InputID::D);
		driver.press();
		CHECK(scene.root.canvas.isAdjustingBrush());
		driver.moveTo(center + Vec2F{ CanvasWidget::kSizeDoublingPixels, 0.0f });
		CHECK_CLOSE(100.0f, scene.brush.size(), 0.5f);
		CHECK_EQUAL(std::string("100 px"), scene.root.toolBar.brushSize.text());
		CHECK(scene.root.canvas.brushCursor().contour);
		bool labelled = false;
		for (const PaintCommand& command : driver.paint().commands())
			if (command.kind == PaintCommand::Kind::Text && command.text == "Size 100 px") labelled = true;
		CHECK(labelled);
		driver.release();
		driver.releaseKey(InputID::D);
		CHECK(!scene.root.canvas.isAdjustingBrush());
		CHECK_EQUAL(before, strokesOf(scene.document));

		driver.drag(center, center + Vec2F{ 40, 0 });
		CHECK_CLOSE(100.0f, scene.brush.size(), 0.5f);
		CHECK(strokesOf(scene.document) > before);
	}

	TEST(HoldingADragsTheAngle) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F center = driver.centerOf(scene.root.canvas);
		driver.moveTo(center);
		driver.pressKey(InputID::A);
		driver.press();
		driver.moveTo(center + Vec2F{ 30, 0 });
		driver.release();
		driver.releaseKey(InputID::A);
		CHECK_CLOSE(30.0f, scene.brush.get(BrushParameter::Angle), 0.01f);
	}

	TEST(FlowAndOpacityHoldsDragTheirAmounts) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.shortcuts().bind(plain(InputID::F), Command::HoldBrushFlow);
		scene.root.commands.shortcuts().bind(plain(InputID::O), Command::HoldBrushOpacity);
		scene.brush.setFlow(0.5f);
		scene.brush.setOpacity(0.5f);
		const Vec2F center = driver.centerOf(scene.root.canvas);
		driver.moveTo(center);
		driver.pressKey(InputID::F);
		driver.press();
		driver.moveTo(center + Vec2F{ -40, 0 });
		driver.release();
		driver.releaseKey(InputID::F);
		CHECK_CLOSE(0.3f, scene.brush.flow(), 1e-3f);
		CHECK_CLOSE(0.5f, scene.brush.opacity(), 1e-4f);
		scene.brush.setOpacity(0.2f);

		driver.moveTo(center);
		driver.pressKey(InputID::O);
		driver.press();
		driver.moveTo(center + Vec2F{ 60, 0 });
		driver.release();
		driver.releaseKey(InputID::O);
		CHECK_CLOSE(0.5f, scene.brush.opacity(), 1e-3f);
	}

	TEST(BrushSizeToolStaysOnUntilToggledOff) {
		ToolsScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.brush.setSize(40);
		scene.root.commands.run(Command::ToolBrushSize);
		CHECK(scene.root.commands.isChecked(Command::ToolBrushSize));
		const Vec2F center = driver.centerOf(scene.root.canvas);
		driver.drag(center, center + Vec2F{ CanvasWidget::kSizeDoublingPixels, 0.0f });
		CHECK_CLOSE(80.0f, scene.brush.size(), 0.5f);
		driver.drag(center, center + Vec2F{ CanvasWidget::kSizeDoublingPixels, 0.0f });
		CHECK_CLOSE(160.0f, scene.brush.size(), 0.5f);
		scene.root.commands.run(Command::ToolBrushSize);
		CHECK(!scene.root.commands.isChecked(Command::ToolBrushSize));
		CHECK(!scene.root.canvas.adjustTool().has_value());
	}
}

SUITE(TabletBrushTools) {
	TEST(TabletSettingsReachTheBrushMenu) {
		TabletScene scene;
		UiDriver driver(scene.studio, { 1280, 800 });
		driver.layout();
		bool found = false;
		for (const auto& entry : scene.classic.mainMenu.items())
			if (entry->entry().text == "Brush" && entry->entry().submenu == &scene.classic.brushMenu) found = true;
		CHECK(found);
		CHECK(findItem(scene.classic.brushMenu, "Reset brush") != nullptr);
	}
}
