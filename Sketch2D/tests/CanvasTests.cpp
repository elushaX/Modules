#include "TestSketchesRoot.hpp"
#include "Checkerboard.hpp"
#include "Sketch2DUI.hpp"
#include "Session.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	class RecordingViewport : public ViewportRenderer {
	public:
		void render(Document& document, const ViewSettings& settings, Vec2F) override {
			lastSettings = settings;
			lastBackground = document.background();
		}
		[[nodiscard]] ImageId image() const override { return { 1 }; }
		void setDebugOverlay(bool) override {}
		[[nodiscard]] bool debugOverlay() const override { return false; }

		ViewSettings lastSettings;
		CanvasBackground lastBackground;
	};

	class CanvasIcons : public IconProvider {
	public:
		[[nodiscard]] ImageId icon(const std::string&) const override { return { 1 }; }
	};

	struct CanvasScene {
		Document document;
		Brush brush;
		RecordingViewport viewport;
		CanvasIcons icons;
		Sketch2DRoot root{ document, viewport, brush, icons, tp::sketch2d::testing::testSketchesRoot() };

		CanvasScene() { root.canvas.settings().viewStiffness = 1; }
	};

	void checkColor(const RGBA& expected, const RGBA& actual) {
		CHECK_CLOSE(expected.r, actual.r, 0.002f);
		CHECK_CLOSE(expected.g, actual.g, 0.002f);
		CHECK_CLOSE(expected.b, actual.b, 0.002f);
		CHECK_CLOSE(expected.a, actual.a, 0.002f);
	}
}

SUITE(CanvasColors) {
	TEST(HueColorConvertsDegreesToRgb) {
		checkColor({ 1, 0, 0, 1 }, HueColor{ 0, 1, 1 }.rgba());
		checkColor({ 0, 1, 0, 1 }, HueColor{ 120, 1, 1 }.rgba());
		checkColor({ 0, 0, 1, 1 }, HueColor{ 240, 1, 1 }.rgba());
		checkColor({ 1, 1, 0, 1 }, HueColor{ 60, 1, 1 }.rgba());
		checkColor({ 0.6f, 0.6f, 0.6f, 1 }, HueColor{ 0, 0, 0.6f }.rgba());
		checkColor({ 1, 0, 0, 0.5f }, HueColor{ 360, 1, 1 }.rgba(0.5f));
	}

	TEST(DefaultsMatchReference) {
		ViewSettings settings;
		checkColor({ 0.6f, 0.6f, 0.6f, 1 }, settings.boardColor.rgba());
		CHECK(settings.canvasShadow);

		Document document;
		checkColor({ 0.12f, 0.12f, 0.12f, 1 }, document.background().rgba());
		CHECK(!document.background().transparent);
	}

	TEST(CanvasWidgetPassesSettingsToRenderer) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.canvas.settings().boardColor = { 200, 0.5f, 0.3f };
		driver.paint();
		CHECK_CLOSE(200, scene.viewport.lastSettings.boardColor.hueDegrees, 0.01f);
	}
}

SUITE(CanvasBackgroundCommand) {
	TEST(DialogAppliesColorAsUndoableStep) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.commands.run(Command::CanvasBackground);
		CanvasBackgroundDialog& dialog = scene.root.canvasBackground;
		CHECK(dialog.isOpen());
		CHECK_EQUAL("Canvas background", dialog.title());

		dialog.wheel.setColor({ 0, 0, 1, 1 });
		driver.click(*dialog.buttons()[0]);
		checkColor({ 0, 0, 1, 1 }, scene.document.background().rgba());
		CHECK_EQUAL("Background", scene.document.history().undoLabel());

		scene.document.undo();
		checkColor({ 0.12f, 0.12f, 0.12f, 1 }, scene.document.background().rgba());
	}

	TEST(TransparentBackgroundAndCancel) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.commands.run(Command::CanvasBackground);
		driver.click(scene.root.canvasBackground.transparent);
		driver.click(*scene.root.canvasBackground.buttons()[0]);
		CHECK(scene.document.background().transparent);
		CHECK_CLOSE(0, scene.document.background().rgba().a, 0.001f);

		scene.root.commands.run(Command::CanvasBackground);
		CHECK(scene.root.canvasBackground.transparent.isChecked());
		driver.click(scene.root.canvasBackground.transparent);
		driver.click(*scene.root.canvasBackground.buttons()[1]);
		CHECK(scene.document.background().transparent);
	}

	TEST(UnchangedBackgroundAddsNoHistory) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasBackground);
		driver.click(*scene.root.canvasBackground.buttons()[0]);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(PickingAColorUpdatesTheCanvasLiveWithoutHistory) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasBackground);
		CanvasBackgroundDialog& dialog = scene.root.canvasBackground;

		driver.settle();
		driver.click(dialog.wheel);

		CHECK(!(scene.document.background() == CanvasBackground{}));
		CHECK_EQUAL(1u, scene.document.history().entries().size());
		driver.paint();
		CHECK(scene.viewport.lastBackground == scene.document.background());

		driver.click(dialog.transparent);
		CHECK(scene.document.background().transparent);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(CancelRestoresTheBackgroundShownBeforeTheDialog) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasBackground);
		CanvasBackgroundDialog& dialog = scene.root.canvasBackground;

		driver.settle();
		driver.click(dialog.wheel);
		CHECK(!(scene.document.background() == CanvasBackground{}));

		driver.click(*dialog.buttons()[1]);
		CHECK(scene.document.background() == CanvasBackground{});

		scene.root.commands.run(Command::CanvasBackground);
		driver.click(dialog.transparent);
		driver.key(InputID::ESCAPE);
		CHECK(!dialog.isOpen());
		CHECK(scene.document.background() == CanvasBackground{});
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(ApplyAfterLivePreviewRecordsOneUndoStep) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasBackground);
		CanvasBackgroundDialog& dialog = scene.root.canvasBackground;
		driver.click(dialog.transparent);
		driver.click(*dialog.buttons()[0]);

		CHECK(scene.document.background().transparent);
		CHECK_EQUAL(2u, scene.document.history().entries().size());
		scene.document.undo();
		CHECK(scene.document.background() == CanvasBackground{});
	}

	TEST(NewFileResetsBackground) {
		CanvasScene scene;
		scene.document.setBackground({ { 30, 1, 1 }, true });
		scene.root.commands.run(Command::NewInfiniteCanvas);
		CHECK(scene.document.background() == CanvasBackground{});
	}
}

SUITE(CanvasAppearanceCommand) {
	TEST(SlidersEditBoardColorLive) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasAppearance);
		CanvasAppearanceDialog& dialog = scene.root.canvasAppearance;
		CHECK(dialog.isOpen());
		CHECK_CLOSE(0.6f, dialog.boardValue.value(), 0.001f);
		CHECK(dialog.shadow.isChecked());

		driver.setSlider(dialog.boardValue.slider(), 0.2f);
		driver.setSlider(dialog.boardHue.slider(), 210);
		driver.setSlider(dialog.boardSaturation.slider(), 0.5f);
		CHECK_CLOSE(0.2f, scene.root.canvas.settings().boardColor.value, 0.02f);
		CHECK_CLOSE(210, scene.root.canvas.settings().boardColor.hueDegrees, 3.0f);

		driver.click(dialog.shadow);
		CHECK(!scene.root.canvas.settings().canvasShadow);

		driver.click(*dialog.buttons()[0]);
		CHECK_CLOSE(0.2f, scene.root.canvas.settings().boardColor.value, 0.02f);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(CancelRestoresPreviousAppearance) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasAppearance);
		driver.setSlider(scene.root.canvasAppearance.boardValue.slider(), 0.9f);
		driver.key(InputID::ESCAPE);
		CHECK_CLOSE(0.6f, scene.root.canvas.settings().boardColor.value, 0.001f);

		scene.root.commands.run(Command::CanvasAppearance);
		driver.setSlider(scene.root.canvasAppearance.boardValue.slider(), 0.9f);
		driver.click(*scene.root.canvasAppearance.buttons()[1]);
		CHECK_CLOSE(0.6f, scene.root.canvas.settings().boardColor.value, 0.001f);
	}

	TEST(CanvasMenuHasReferenceEntries) {
		CanvasScene scene;
		scene.root.canvasMenu.refresh();
		CHECK_EQUAL("Canvas frame size", scene.root.canvasMenu.items()[0]->text());
		CHECK_EQUAL("Canvas background", scene.root.canvasMenu.items()[1]->text());
		CHECK_EQUAL("Canvas appearance", scene.root.canvasMenu.items()[2]->text());
		CHECK_EQUAL("Infinite canvas", scene.root.canvasMenu.items()[3]->text());
	}
}

SUITE(TransparencyCheckerboard) {
	TEST(CellsAlternateAndFollowPan) {
		Document document;
		const std::vector<CheckerCell> cells = checkerCells(document, { 64, 32 });
		CHECK(cells.size() > 1);
		checkColor(kCheckerLight, cells.front().color);
		CHECK_CLOSE(64, cells.front().size.x, 0.001f);

		for (size_t index = 1; index < cells.size(); index++) {
			checkColor(kCheckerDark, cells[index].color);
			const int4 column = (int4) std::lround(cells[index].position.x / kCheckerCellPixels);
			const int4 row = (int4) std::lround(cells[index].position.y / kCheckerCellPixels);
			CHECK_EQUAL(1, ((column + row) % 2 + 2) % 2);
		}

		document.pan(Vec2F(kCheckerCellPixels, 0.0f));
		const std::vector<CheckerCell> panned = checkerCells(document, { 64, 32 });
		bool shifted = false;
		for (const CheckerCell& cell : panned) {
			if (cell.position.x == 0 && cell.position.y == 0 && cell.color.r == kCheckerDark.r) shifted = true;
		}
		CHECK(shifted);
	}
}

#include "CanvasFrameGeometry.hpp"

namespace {
	void paintSquare(Document& document, int4 left, int4 top, int4 size) {
		for (int4 y = top; y < top + size; y++) {
			for (int4 x = left; x < left + size; x++) {
				const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
				uint1* pixel = document.activeLayer()->tileAt(coord)->pixels() +
											 ((ualni) (y - coord.y * kTileSize) * kTileSize + (ualni) (x - coord.x * kTileSize)) * 4;
				pixel[0] = pixel[3] = 255;
			}
		}
	}
}

SUITE(CanvasFrame) {
	TEST(NewDocumentsAreInfinite) {
		Document document;
		CHECK(document.isInfinite());
		CHECK(document.frame().isEmpty());
	}

	TEST(EmptyCanvasGetsFiveHundredSquareFrameAroundView) {
		Document document;
		CHECK(document.setInfinite(false, { 1000, -40 }));
		const CanvasFrame& frame = document.frame();
		CHECK(!frame.infinite);
		CHECK_EQUAL(750, frame.left);
		CHECK_EQUAL(-290, frame.top);
		CHECK_EQUAL(500, frame.width());
		CHECK_EQUAL(500, frame.height());
	}

	TEST(FrameFitsPaintedContent) {
		Document document;
		paintSquare(document, -30, 10, 20);
		paintSquare(document, 300, 400, 5);
		document.setInfinite(false);
		const CanvasFrame& frame = document.frame();
		CHECK_EQUAL(-30, frame.left);
		CHECK_EQUAL(10, frame.top);
		CHECK_EQUAL(305, frame.right);
		CHECK_EQUAL(405, frame.bottom);
	}

	TEST(FrameKeepsItsRectAcrossToggles) {
		Document document;
		document.setInfinite(false, { 0, 0 });
		document.setFrame(0, 0, 800, 600);
		document.setInfinite(true);
		CHECK(document.isInfinite());
		document.setInfinite(false, { 9999, 9999 });
		CHECK_EQUAL(800, document.frame().width());
		CHECK_EQUAL(0, document.frame().left);
	}

	TEST(FrameIsCappedAndClamped) {
		Document document;
		paintSquare(document, 0, 0, 1);
		paintSquare(document, 60000, 0, 1);
		document.setInfinite(false);
		CHECK_EQUAL(50000, document.frame().width());

		document.setFrame(-0x300000, 0, 0x300000, 10);
		CHECK_EQUAL(-0x200000, document.frame().left);
		CHECK_EQUAL(0x200000, document.frame().right);
		document.setFrame(5, 5, 2, 2);
		CHECK_EQUAL(6, document.frame().right);
		CHECK_EQUAL(6, document.frame().bottom);
	}

	TEST(FrameIsPartOfHistory) {
		Document document;
		document.setInfinite(false, { 0, 0 });
		document.commit("Infinite canvas");
		document.setFrame(0, 0, 100, 100);
		document.commit("Canvas frame size");

		document.undo();
		CHECK_EQUAL(500, document.frame().width());
		document.undo();
		CHECK(document.isInfinite());
	}

	TEST(InfiniteCommandTogglesAndIsChecked) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		CHECK(scene.root.commands.isChecked(Command::InfiniteCanvas));

		scene.root.commands.run(Command::InfiniteCanvas);
		CHECK(!scene.document.isInfinite());
		CHECK(!scene.root.commands.isChecked(Command::InfiniteCanvas));
		CHECK_EQUAL("Infinite canvas", scene.document.history().undoLabel());

		scene.root.commands.run(Command::InfiniteCanvas);
		CHECK(scene.document.isInfinite());
	}

	TEST(FrameSizeDialogResizesAroundCenter) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.document.setInfinite(false, { 0, 0 });

		scene.root.commands.run(Command::CanvasFrameSize);
		CanvasFrameSizeDialog& dialog = scene.root.canvasFrameSize;
		CHECK(dialog.isOpen());
		CHECK_EQUAL("500 px", dialog.width.field().text());

		dialog.width.setValue(1920);
		dialog.height.setValue(1080);
		driver.click(*dialog.buttons()[0]);
		const CanvasFrame& frame = scene.document.frame();
		CHECK_EQUAL(1920, frame.width());
		CHECK_EQUAL(1080, frame.height());
		CHECK_EQUAL(-960, frame.left);
		CHECK_EQUAL(-540, frame.top);
		CHECK_EQUAL("Canvas frame size", scene.document.history().undoLabel());
	}

	TEST(FrameSizeOnInfiniteCanvasMakesItFramed) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.commands.run(Command::CanvasFrameSize);
		CHECK(scene.document.isInfinite());
		driver.click(*scene.root.canvasFrameSize.buttons()[0]);
		CHECK(!scene.document.isInfinite());
		CHECK_EQUAL(500, scene.document.frame().width());
	}
}

SUITE(CanvasFrameGeometry) {
	TEST(VisibleAreaIsFrameClippedToViewport) {
		Document document;
		CHECK_CLOSE(1280, visibleCanvasArea(document, { 1280, 800 }).size.x, 0.01f);

		document.setInfinite(false, { 0, 0 });
		document.setFrame(-100, -50, 100, 50);
		document.pan({ 300, 200 });
		const RectF area = visibleCanvasArea(document, { 1280, 800 });
		const Vec2F topLeft = document.worldToScreen({ -100, -50 });
		CHECK_CLOSE(topLeft.x, area.pos.x, 0.01f);
		CHECK_CLOSE(200, area.size.x, 0.01f);
		CHECK_CLOSE(100, area.size.y, 0.01f);

		document.pan({ -5000, 0 });
		CHECK_CLOSE(0, visibleCanvasArea(document, { 1280, 800 }).size.x, 0.01f);
	}

	TEST(ShadowBandsSurroundFrameAndFadeOut) {
		const std::vector<ShadowBand> bands = canvasShadow({ 100, 100, 200, 150 });
		CHECK_EQUAL((size_t) kCanvasShadowSteps, bands.size());
		halnf total = 0;
		for (size_t index = 0; index < bands.size(); index++) {
			const RectF& rect = bands[index].rect;
			CHECK(rect.pos.x < 100 && rect.pos.x + rect.size.x > 300);
			CHECK(rect.pos.y + rect.size.y > 250);
			if (index > 0) CHECK(rect.size.x < bands[index - 1].rect.size.x);
			if (index > 0) CHECK(bands[index].color.a >= bands[index - 1].color.a);
			total += bands[index].color.a;
		}
		CHECK(total > 0.1f && total <= kCanvasShadowAlpha);
	}
}

#include "ChannelView.hpp"

SUITE(ChannelView) {
	TEST(ColorChannelsBecomeGreyKeepingAlpha) {
		const RGBA orange = { 1.0f * 0.5f, 0.5f * 0.5f, 0.0f, 0.5f };
		checkColor({ 0.5f, 0.5f, 0.5f, 0.5f }, applyChannelView(ViewChannel::Red, orange));
		checkColor({ 0.25f, 0.25f, 0.25f, 0.5f }, applyChannelView(ViewChannel::Green, orange));
		checkColor({ 0, 0, 0, 0.5f }, applyChannelView(ViewChannel::Blue, orange));
	}

	TEST(LightnessUsesReferenceLumaWeights) {
		CHECK_CLOSE(0.299f, kLumaRed, 0.001f);
		CHECK_CLOSE(0.587f, kLumaGreen, 0.001f);
		CHECK_CLOSE(0.114f, kLumaBlue, 0.001f);
		const RGBA lightness = applyChannelView(ViewChannel::Lightness, { 0.2f, 0.4f, 0.6f, 1 });
		CHECK_CLOSE(0.2f * kLumaRed + 0.4f * kLumaGreen + 0.6f * kLumaBlue, lightness.r, 0.0001f);
		CHECK_CLOSE(lightness.r, lightness.b, 0.0001f);
	}

	TEST(AlphaViewsShowCoverage) {
		checkColor(
			{ 0.75f, 0.75f, 0.75f, 0.75f }, applyChannelView(ViewChannel::AlphaOnBlack, { 0.1f, 0.2f, 0.3f, 0.75f })
		);
		checkColor({ 0, 0, 0, 0.75f }, applyChannelView(ViewChannel::AlphaOnWhite, { 0.1f, 0.2f, 0.3f, 0.75f }));
		checkColor({ 0, 0, 0, 1 }, channelBackground(ViewChannel::AlphaOnBlack, { 1, 1, 1, 1 }));
		checkColor({ 1, 1, 1, 1 }, channelBackground(ViewChannel::AlphaOnWhite, { 0, 0, 1, 1 }));
	}

	TEST(BackgroundGoesThroughSameChannel) {
		checkColor({ 0, 0, 0, 1 }, channelBackground(ViewChannel::Red, { 0, 0, 1, 1 }));
		checkColor({ 1, 1, 1, 1 }, channelBackground(ViewChannel::Blue, { 0, 0, 1, 1 }));
	}

	TEST(ShowChannelCommandTogglesAndShortcut) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		CHECK(!scene.root.canvas.settings().showChannel);

		Modifiers control;
		control.control = true;
		driver.key(InputID::P, control);
		CHECK(scene.root.canvas.settings().showChannel);
		CHECK(scene.root.commands.isChecked(Command::ShowChannel));
		driver.paint();
		CHECK(scene.viewport.lastSettings.showChannel);

		scene.root.commands.run(Command::ShowChannel);
		CHECK(!scene.root.canvas.settings().showChannel);
		CHECK_EQUAL(1u, scene.document.history().entries().size());
	}

	TEST(ChannelSubmenuPicksAndShowsChannel) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.root.channelMenu.refresh();
		CHECK_EQUAL((size_t) kViewChannelCount, scene.root.channelMenu.items().size());
		CHECK_EQUAL("Lightness", scene.root.channelMenu.items()[5]->text());

		scene.root.channelMenu.items()[2]->activate();
		CHECK(scene.root.canvas.settings().showChannel);
		CHECK(scene.root.canvas.settings().channel == ViewChannel::Blue);
		scene.root.channelMenu.refresh();
		CHECK(scene.root.channelMenu.items()[2]->entry().isChecked());
		CHECK(!scene.root.channelMenu.items()[0]->entry().isChecked());
	}
}

SUITE(ViewRotation) {
	void checkPoint(Vec2F expected, Vec2F actual) {
		CHECK_CLOSE(expected.x, actual.x, 0.01f);
		CHECK_CLOSE(expected.y, actual.y, 0.01f);
	}

	TEST(ScreenAndWorldStayInverseWhenRotated) {
		Document document;
		document.setZoom(1.7f);
		document.pan({ 120, -40 });
		document.setRotation(0.6f);

		for (Vec2F screen : { Vec2F(0.0f, 0.0f), Vec2F(300.0f, 50.0f), Vec2F(-20.0f, 700.0f) })
			checkPoint(screen, document.worldToScreen(document.screenToWorld(screen)));
	}

	TEST(RotatingAboutAPointKeepsThatPointStill) {
		Document document;
		document.setZoom(2);
		const Vec2F pivot = { 400, 300 };
		const Vec2F world = document.screenToWorld(pivot);

		document.rotateAt(pivot, 1.1f);

		checkPoint(world, document.screenToWorld(pivot));
		CHECK_CLOSE(1.1f, document.rotation(), 0.0001f);
	}

	TEST(QuarterTurnRotatesWorldAxesOnScreen) {
		Document document;
		document.setRotation(1.5707963f);

		checkPoint({ 0, 10 }, document.worldToScreen({ 10, 0 }));
	}

	TEST(PanningFollowsTheFingerWhenRotated) {
		Document document;
		document.setRotation(0.9f);
		const Vec2F world = document.screenToWorld({ 200, 200 });

		document.pan({ 30, -50 });

		checkPoint({ 230, 150 }, document.worldToScreen(world));
	}

	TEST(RotationWrapsAndResetsWithTheView) {
		Document document;
		document.setRotation(7.0f);
		CHECK_CLOSE(7.0f - 6.2831853f, document.rotation(), 0.0001f);

		document.resetView();
		CHECK_EQUAL(0.0f, document.rotation());
	}

	TEST(VisibleAreaCoversTheRotatedScreen) {
		Document document;
		document.setRotation(0.7853982f);

		const RectF area = visibleCanvasArea(document, { 100, 100 });

		CHECK_CLOSE(141.42f, area.size.x, 0.05f);
		CHECK_CLOSE(141.42f, area.size.y, 0.05f);
	}

	TEST(TwoFingerTwistRotatesTheCanvasAndSmallTwistsSnapBack) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		const Vec2F center = scene.root.canvas.windowRect().center();

		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, center, { 0, 0 }, 1.0f, 0.5f }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, center }));
		CHECK_CLOSE(0.5f, scene.document.rotation(), 0.0001f);

		driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, center }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::Update, center, { 0, 0 }, 1.0f, -0.46f }));
		driver.ui().dispatch(Event::gesture({ GesturePhase::End, center }));
		CHECK_EQUAL(0.0f, scene.document.rotation());
	}

	TEST(ViewCommandsRotateAndReset) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.commands.run(Command::RotateRight);
		CHECK_CLOSE(kRotationStepRadians, scene.document.rotation(), 0.0001f);
		scene.root.commands.run(Command::RotateLeft);
		scene.root.commands.run(Command::RotateLeft);
		CHECK_CLOSE(-kRotationStepRadians, scene.document.rotation(), 0.0001f);

		scene.root.commands.run(Command::ResetRotation);
		CHECK_EQUAL(0.0f, scene.document.rotation());
	}

	TEST(PaintingLandsUnderThePenWhenRotated) {
		CanvasScene scene;
		UiDriver driver(scene.root, { 1280, 800 });
		scene.document.setRotation(0.8f);
		const Vec2F point = scene.root.canvas.windowRect().center();
		const Vec2F world = scene.document.screenToWorld(point - scene.root.canvas.windowRect().pos);

		driver.drag(point, point + Vec2F(1.0f, 0.0f), 2);
		scene.document.settlePaint();

		const TileCoord coord = { floordiv((int4) std::floor(world.x), kTileSize),
															floordiv((int4) std::floor(world.y), kTileSize) };
		const Tile* tile = scene.document.activeLayer()->tileIfExists(coord);
		REQUIRE CHECK(tile != nullptr);
		const int4 x = (int4) std::floor(world.x) - coord.x * kTileSize;
		const int4 y = (int4) std::floor(world.y) - coord.y * kTileSize;
		CHECK(tile->pixels()[((ualni) y * kTileSize + (ualni) x) * 4 + 3] > 0);
	}

	TEST(SessionsRememberTheRotation) {
		Document document;
		Brush brush;
		ViewSettings settings;
		document.setRotation(-1.25f);

		const std::optional<SessionSnapshot> parsed =
			SessionSnapshot::parse(SessionSnapshot::capture(document, brush, settings, {}).serialize());
		REQUIRE CHECK(parsed.has_value());
		Document restored;
		parsed->applyTo(restored, brush, settings);
		CHECK_CLOSE(-1.25f, restored.rotation(), 0.0001f);
	}

	TEST(ViewChangesEaseTowardsTheirTarget) {
		const auto twist = [](CanvasScene& scene, UiDriver& driver) {
			const Vec2F point = scene.root.canvas.windowRect().pos + Vec2F(300, 200);
			driver.ui().dispatch(Event::gesture({ GesturePhase::Begin, point }));
			driver.ui().dispatch(Event::gesture({ GesturePhase::Update, point, { 40, 0 }, 2.0f, 0.5f }));
			driver.ui().dispatch(Event::gesture({ GesturePhase::End, point }));
		};
		CanvasScene instant;
		UiDriver instantDriver(instant.root, { 1280, 800 });
		twist(instant, instantDriver);

		CanvasScene scene;
		scene.root.canvas.settings().viewStiffness = 0.5f;
		UiDriver driver(scene.root, { 1280, 800 });
		twist(scene, driver);

		CHECK(scene.root.canvas.isViewMoving());
		CHECK_CLOSE(1.0f, scene.document.zoom(), 0.001f);
		driver.advance(16);
		const halnf early = scene.document.zoom();
		CHECK(early > 1.0f && early < 2.0f);
		driver.advance(3000);

		CHECK(!scene.root.canvas.isViewMoving());
		CHECK_CLOSE(2.0f, scene.document.zoom(), 0.0001f);
		CHECK_CLOSE(0.5f, scene.document.rotation(), 0.0001f);
		CHECK_CLOSE(instant.document.panWorld().x, scene.document.panWorld().x, 0.001f);
		CHECK_CLOSE(instant.document.panWorld().y, scene.document.panWorld().y, 0.001f);
	}

	TEST(StifferViewSettlesFaster) {
		const auto progressAfter = [](halnf stiffness) {
			CanvasScene scene;
			scene.root.canvas.settings().viewStiffness = stiffness;
			UiDriver driver(scene.root, { 1280, 800 });
			scene.root.canvas.changeView([](Document& view) { view.setZoom(2.0f); });
			driver.advance(48);
			return scene.document.zoom() - 1.0f;
		};
		CHECK(progressAfter(0.9f) > progressAfter(0.2f));
	}

	TEST(ChangesDuringMotionRetargetFromTheFinalView) {
		CanvasScene scene;
		scene.root.canvas.settings().viewStiffness = 0.5f;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.canvas.changeView([](Document& view) { view.setZoom(view.zoom() * 2.0f); });
		driver.advance(16);
		scene.root.canvas.changeView([](Document& view) { view.setZoom(view.zoom() * 2.0f); });
		driver.advance(3000);

		CHECK_CLOSE(4.0f, scene.document.zoom(), 0.0001f);
	}

	TEST(DirectViewChangesStopTheMotion) {
		CanvasScene scene;
		scene.root.canvas.settings().viewStiffness = 0.5f;
		UiDriver driver(scene.root, { 1280, 800 });

		scene.root.canvas.changeView([](Document& view) { view.setZoom(3.0f); });
		driver.advance(16);
		scene.document.resetView();
		driver.advance(3000);

		CHECK(!scene.root.canvas.isViewMoving());
		CHECK_CLOSE(1.0f, scene.document.zoom(), 0.0001f);
	}

	TEST(FullStiffnessMovesTheViewInstantly) {
		CanvasScene scene;
		scene.root.canvas.changeView([](Document& view) { view.setZoom(2.0f); });

		CHECK(!scene.root.canvas.isViewMoving());
		CHECK_CLOSE(2.0f, scene.document.zoom(), 0.0001f);
	}
}
