#include "AppDriver.hpp"
#include "Sketch2DApplication.hpp"
#include "StudioUI.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <fstream>
#include <unistd.h>

using namespace tp;
using namespace tp::sketch2d;
using namespace tp::ui;

namespace {
	std::filesystem::path testStorageRoot() {
		return std::filesystem::temp_directory_path() / ("sketch2d-app-test-" + std::to_string(::getpid()));
	}

	Sketch2DOptions testOptions() { return Sketch2DOptions::forTesting(testStorageRoot().string()); }

	struct TestStorage {
		~TestStorage() {
			std::error_code error;
			std::filesystem::remove_all(testStorageRoot(), error);
		}
	};

	struct AppScene {
		TestStorage storage;
		Sketch2DApplication application{ testOptions() };
		AppDriver driver{ application };

		Sketch2DRoot& root() { return application.root(); }
		StudioRoot& studio() { return *application.studioRoot(); }
		Document& document() { return application.document(); }

		Vec2F canvasPoint(halnf x, halnf y) {
			const RectF canvas = root().canvas.windowRect();
			return canvas.pos + Vec2F(x, y);
		}
	};
}

SUITE(Sketch2DApplicationIntegration) {
	TEST(HungFrameLeavesAReportNamingTheScopeAndEngineState) {
		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "sketch2d-app-hang-test";
		std::filesystem::remove_all(directory);
		setenv("MODULES_HANG_DIR", directory.c_str(), 1);
		{
			AppScene scene;
			REQUIRE CHECK(scene.application.hangWatchdog() != nullptr);
			bool simulated = false;
			for (const auto& item : scene.root().debugMenu.items())
				if (item->entry().text == "Simulate hang (5 s)") {
					item->entry().action();
					simulated = true;
				}
			REQUIRE CHECK(simulated);
			scene.driver.frame();
			REQUIRE CHECK_EQUAL(1u, scene.application.hangWatchdog()->reports().size());
			const std::filesystem::path report = scene.application.hangWatchdog()->reports().front();
			std::ifstream summaryFile(report / "summary.txt");
			const std::string summary((std::istreambuf_iterator<char>(summaryFile)), std::istreambuf_iterator<char>());
			CHECK(summary.find("debug.simulated_hang") != std::string::npos);
			CHECK(summary.find("verdict: blocked") != std::string::npos);
			CHECK(std::filesystem::exists(report / "engine.txt"));
			CHECK(std::filesystem::exists(report / "threads.txt"));
			CHECK(report.filename().string().rfind("sketch2d-", 0) == 0);
		}
		unsetenv("MODULES_HANG_DIR");
		std::filesystem::remove_all(directory);
	}

	TEST(StrokePaintsTilesAndUploadsThem) {
		AppScene scene;
		scene.driver.drag(scene.canvasPoint(200, 200), scene.canvasPoint(420, 260));

		CHECK(scene.document().tileCount() > 0);
		CHECK_EQUAL("Paint", scene.document().history().undoLabel());
		scene.driver.frame();
		CHECK(scene.application.renderer().drawnTilesLastFrame() > 0);
	}

	TEST(UndoButtonRemovesStroke) {
		AppScene scene;
		scene.driver.drag(scene.canvasPoint(200, 200), scene.canvasPoint(300, 220));
		CHECK(scene.document().tileCount() > 0);

		scene.driver.click(scene.studio().undo);
		CHECK_EQUAL(0u, scene.document().tileCount());
	}

	TEST(SwitchingLayersWithThumbnailsDoesNotCrash) {
		AppScene scene;
		scene.driver.drag(scene.canvasPoint(200, 200), scene.canvasPoint(300, 220));
		scene.driver.click(scene.studio().layers);
		LayersPopup& popup = scene.root().toolBar.layersPopup;

		scene.driver.click(popup.add);
		scene.driver.drag(scene.canvasPoint(150, 300), scene.canvasPoint(260, 320));
		scene.driver.click(scene.studio().layers);
		for (int round = 0; round < 3; round++) {
			for (size_t row = 0; row < popup.rows().size(); row++)
				scene.driver.click(popup.rows()[row]->name);
		}
		CHECK_EQUAL(2u, popup.rows().size());
	}

	TEST(DraggingLayerGripReorders) {
		AppScene scene;
		Layer* bottom = scene.document().activeLayer();
		scene.driver.click(scene.studio().layers);
		LayersPopup& popup = scene.root().toolBar.layersPopup;
		scene.driver.click(popup.add);
		scene.driver.frames(20);

		ReorderableList& list = popup.list();
		REQUIRE CHECK_EQUAL(2u, list.itemCount());
		const Vec2F grip = list.windowPosition() + list.gripRect(1).center();
		const Vec2F above = list.windowPosition() + list.gripRect(0).center() - Vec2F(0.0f, 20.0f);
		scene.driver.drag(grip, above);
		CHECK(scene.document().layers()[1].get() == bottom);
	}

	TEST(ZoomedOutRenderingUsesMips) {
		AppScene scene;
		scene.driver.drag(scene.canvasPoint(200, 200), scene.canvasPoint(420, 260));
		for (int notch = 0; notch < 20; notch++)
			scene.driver.scroll(scene.canvasPoint(300, 300), -1);
		scene.driver.frame();
		CHECK(scene.document().zoom() < 0.5f);
		CHECK(scene.application.renderer().mipLevelLastFrame() >= 1);
		CHECK(scene.application.renderer().drawnTilesLastFrame() > 0);
	}

	TEST(FramedCanvasRendersAfterToggle) {
		AppScene scene;
		scene.root().commands.run(Command::InfiniteCanvas);
		scene.driver.frames(3);
		CHECK(!scene.document().isInfinite());
		scene.driver.drag(scene.canvasPoint(300, 300), scene.canvasPoint(340, 320));
		CHECK(scene.document().tileCount() > 0);
	}
}

SUITE(Sketch2DTileUploads) {
	void paintTiles(Document & document, int4 columns, int4 rows, int4 offsetX) {
		Brush brush;
		brush.setSize(40);
		for (int4 y = 0; y < rows; y++)
			for (int4 x = 0; x < columns; x++)
				document.stampBrushDab(
					{ (halnf) ((x + offsetX) * kTileSize + kTileSize / 2), (halnf) (y * kTileSize + kTileSize / 2) }, brush
				);
		document.finishStroke();
	}

	TEST(OffScreenTilesAreNotUploaded) {
		AppScene scene;
		paintTiles(scene.document(), 2, 2, 0);
		paintTiles(scene.document(), 2, 2, 40);
		scene.driver.frames(3);
		CHECK_EQUAL(0u, scene.application.renderer().uploadedTilesLastFrame());
		CHECK(scene.application.renderer().drawnTilesLastFrame() == 4);
		CHECK(!scene.application.renderer().hasPendingUploads());
	}

	TEST(LargeUploadsAreSpreadOverFrames) {
		AppScene scene;
		TileRenderer& renderer = scene.application.renderer();
		renderer.setUploadTilesPerFrame(3);
		paintTiles(scene.document(), 3, 3, 0);
		scene.driver.frame();
		CHECK_EQUAL(3u, renderer.uploadedTilesLastFrame());
		CHECK(renderer.drawnTilesLastFrame() + renderer.fallbackTilesLastFrame() > 0);
		CHECK(renderer.hasPendingUploads());

		int frames = 0;
		while (renderer.hasPendingUploads() && frames < 20) {
			scene.driver.frame();
			frames++;
		}
		CHECK(!renderer.hasPendingUploads());
		CHECK_EQUAL(9u, renderer.drawnTilesLastFrame());
		CHECK(frames >= 2);
	}
}

SUITE(Sketch2DChannelViewIntegration) {
	TEST(ChannelViewRendersEveryChannel) {
		AppScene scene;
		scene.driver.drag(scene.canvasPoint(200, 200), scene.canvasPoint(420, 260));
		scene.root().commands.run(Command::ShowChannel);
		CHECK(scene.root().canvas.settings().showChannel);

		for (int index = 0; index < kViewChannelCount; index++) {
			scene.root().channelMenu.items()[(size_t) index]->activate();
			scene.driver.frame();
			CHECK(scene.application.renderer().drawnTilesLastFrame() > 0);
		}
	}
}

SUITE(Sketch2DInputCoalescing) {
	TEST(DrawingKeepsEveryPointerSample) {
		AppScene scene;
		const CoalescingStatistics before = scene.application.window().inputStatistics();

		WindowInputObserver& input = scene.application.window().injectInput();
		for (int step = 0; step < 20; step++)
			input.onPointerMoved(scene.canvasPoint(200.0f + (halnf) step * 5, 200.0f));
		scene.driver.frame();

		const CoalescingStatistics after = scene.application.window().inputStatistics();
		CHECK_EQUAL(20u, after.delivered - before.delivered);
	}
}

SUITE(Sketch2DPenInput) {
	int4 paintedHeightAt(Document & document, int4 worldX) {
		document.settlePaint();
		int4 height = 0;
		for (int4 y = -400; y < 400; y++) {
			const Tile* tile = document.activeLayer()->tileIfExists({ floordiv(worldX, kTileSize), floordiv(y, kTileSize) });
			if (!tile) continue;
			const int4 lx = worldX - floordiv(worldX, kTileSize) * kTileSize;
			const int4 ly = y - floordiv(y, kTileSize) * kTileSize;
			height += tile->pixels()[((ualni) ly * kTileSize + lx) * 4 + 3] > 0 ? 1 : 0;
		}
		return height;
	}

	TEST(PenPressureWidensTheStroke) {
		AppScene scene;
		Brush& brush = scene.application.brush();
		brush.setSize(40);
		brush.setHardness(1);
		brush.setUsesPressure(PressureTarget::Size, true);

		std::vector<Vec2F> points;
		std::vector<PointerSample> samples;
		for (int step = 0; step <= 20; step++) {
			points.push_back(scene.canvasPoint(200.0f + (halnf) step * 15, 300.0f));
			samples.push_back({ PointerTool::Pen, 0.1f + 0.9f * (halnf) step / 20.0f, { 20, 10 } });
		}
		scene.driver.penStroke(points, samples);

		Document& document = scene.document();
		const Vec2F start = document.screenToWorld(scene.root().canvas.toLocal(points[2]));
		const Vec2F end = document.screenToWorld(scene.root().canvas.toLocal(points[18]));
		const int4 thin = paintedHeightAt(document, (int4) start.x);
		const int4 thick = paintedHeightAt(document, (int4) end.x);
		CHECK(thin > 0);
		CHECK(thick > thin * 2);
	}
}

SUITE(Sketch2DSessionRecordingIntegration) {
	TEST(RecordedSessionCapturesRealWindowInputAndRestoresTheCanvas) {
		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "sketch2d-app-sessions";
		std::filesystem::remove_all(directory);
		setenv("MODULES_SESSION_DIR", directory.c_str(), 1);

		AppScene scene;
		scene.driver.drag(scene.canvasPoint(200, 200), scene.canvasPoint(300, 220));
		const ualni tilesBefore = scene.document().tileCount();

		scene.root().commands.run(Command::RecordSession);
		REQUIRE CHECK(scene.application.debugTools().recording().badge.isVisible());
		PointerSample pen;
		pen.tool = PointerTool::Pen;
		pen.pressure = 0.6f;
		scene.driver.penStroke(
			{ scene.canvasPoint(150, 320), scene.canvasPoint(250, 330), scene.canvasPoint(350, 350) }, { pen, pen, pen }
		);
		scene.root().commands.run(Command::StopSessionRecording);
		scene.driver.settle(scene.application.debugTools().recording().saveDialog);
		REQUIRE CHECK(scene.application.debugTools().recording().saveDialog.isOpen());
		scene.driver.key(InputID::ENTER);

		const std::optional<RecordedSession> saved =
			RecordedSession::load(scene.application.debugTools().recording().lastSessionDirectory());
		REQUIRE CHECK(saved.has_value());
		size_t penSamples = 0;
		for (const RecordedInput& event : saved->input.events)
			if (event.kind == RecordedInput::Kind::Sample && event.sample.isStylus()) penSamples++;
		CHECK(penSamples >= 3);
		CHECK(saved->recordedFrames.find("frame.draw") != std::string::npos);
		CHECK(!saved->snapshot.environment.device.empty());

		Document restored;
		Brush brush;
		ViewSettings settings;
		saved->snapshot.applyTo(restored, brush, settings);
		CHECK_EQUAL(tilesBefore, restored.tileCount());

		unsetenv("MODULES_SESSION_DIR");
		if (!std::getenv("SKETCH2D_KEEP_TEST_SESSION")) std::filesystem::remove_all(directory);
	}

	TEST(PressingDDoesNotOpenTheDebugUi) {
		AppScene scene;

		scene.driver.key(InputID::D);
		scene.driver.frame();

		CHECK(!scene.application.inspector().isVisible());
	}

	TEST(DebugMenuTogglesTheDebugUi) {
		AppScene scene;
		const MenuItem* toggle = nullptr;
		for (const auto& item : scene.root().debugMenu.items())
			if (item->entry().text == "Debug UI") toggle = item.get();
		REQUIRE CHECK(toggle != nullptr);

		toggle->entry().action();
		CHECK(scene.application.inspector().isVisible());
		CHECK(toggle->entry().isChecked());

		toggle->entry().action();
		CHECK(!scene.application.inspector().isVisible());
	}

	TEST(TabletLayoutCanBeForcedOnDesktop) {
		TestStorage storage;
		Sketch2DApplication application(testOptions());
		AppDriver driver(application);
		driver.frame();

		REQUIRE CHECK(application.studioRoot() != nullptr);
		CHECK(application.studioRoot()->toolBar.isVisible());
		CHECK(application.studioRoot()->activeBar.isVisible());
		CHECK(!application.root().toolBar.isVisible());
	}
}

SUITE(Sketch2DDebugView) {
	TEST(DebugViewCountsTilesOfTheShownLevelByState) {
		AppScene scene;
		scene.application.renderer().setDebug(true);
		Layer* layer = scene.document().activeLayer();
		for (int4 y = 0; y < 8; y++)
			for (int4 x = 0; x < 8; x++)
				layer->tileAt({ x, y })->clear({ 0, 0, 1, 1 });
		scene.document().commit("Fill");
		scene.driver.frames(20);
		const ualni doneAtFullSize = scene.application.renderer().debugCount(TileDebugState::Done);
		CHECK(doneAtFullSize > 0);

		for (int notch = 0; notch < 30; notch++)
			scene.driver.scroll(scene.canvasPoint(300, 300), -1);
		scene.driver.frames(20);
		const int4 level = scene.application.renderer().mipLevelLastFrame();
		REQUIRE CHECK(level >= 2);
		const ualni done = scene.application.renderer().debugCount(TileDebugState::Done);
		CHECK(done > 0);
		CHECK(done <= (ualni) std::max(1, 64 >> (2 * level)) + 4);

		Brush brush;
		brush.setSize(400);
		for (int4 dab = 0; dab < 6; dab++)
			scene.document().stampBrushDab({ 100.0f + (halnf) dab * 120.0f, 500.0f }, brush);
		scene.driver.frames(1);
		CHECK(scene.application.renderer().debugCount(TileDebugState::Preview) > 0);
		scene.document().finishStroke();
		scene.document().settlePaint();
		scene.driver.frames(20);
		CHECK_EQUAL(0u, scene.application.renderer().debugCount(TileDebugState::Preview));
		CHECK_EQUAL(0u, scene.application.renderer().debugCount(TileDebugState::Queued));
		CHECK_EQUAL(0u, scene.application.renderer().debugCount(TileDebugState::Working));

		scene.document().setRotation(0.6f);
		scene.driver.frames(20);
		CHECK(scene.application.renderer().debugCount(TileDebugState::Done) > 0);
		CHECK(scene.application.renderer().drawnTilesLastFrame() > 0);
		scene.application.renderer().setDebug(false);
	}
}

SUITE(Sketch2DZoomedOutStrokes) {
	TEST(ZoomedOutStrokeShowsAtOnceAndSettlesToFullResolution) {
		AppScene scene;
		Layer* layer = scene.document().activeLayer();
		for (int4 y = 0; y < 8; y++)
			for (int4 x = 0; x < 8; x++)
				layer->tileAt({ x, y })->clear({ 0, 0, 1, 1 });
		scene.document().commit("Fill");
		for (int notch = 0; notch < 30; notch++)
			scene.driver.scroll(scene.canvasPoint(300, 300), -1);
		scene.driver.frames(10);
		REQUIRE CHECK(scene.document().zoom() <= 0.25f);
		const RGBA before = scene.document().sampleColor({ 450.5f, 410.5f }, false);
		Brush brush;
		brush.setSize(80);
		brush.setHardness(1.0f);
		brush.color = { 1, 1, 0, 1 };
		for (int4 dab = 0; dab <= 10; dab++)
			scene.document().stampBrushDab({ 200.0f + (halnf) dab * 50.0f, 410.0f }, brush);
		CHECK(scene.document().deferredDabs() > 0);
		scene.driver.frames(2);
		CHECK(scene.application.renderer().drawnTilesLastFrame() > 0);
		scene.document().finishStroke();
		scene.driver.frames(30);
		scene.document().settlePaint();
		CHECK_EQUAL("Paint", scene.document().history().undoLabel());
		const RGBA after = scene.document().sampleColor({ 450.5f, 410.5f }, false);
		CHECK(std::abs(after.r - before.r) + std::abs(after.g - before.g) + std::abs(after.b - before.b) > 0.1f);
		const RGBA away = scene.document().sampleColor({ 450.5f, 900.5f }, false);
		CHECK_CLOSE(before.b, away.b, 0.01f);
		CHECK_EQUAL(0u, scene.document().paintBacklog());
		CHECK(scene.application.renderer().drawnTilesLastFrame() > 0);
	}
}

SUITE(Sketch2DUploadBudget) {
	void fillArea(Document & document, int4 tiles) {
		Layer* layer = document.activeLayer();
		for (int4 y = 0; y < tiles; y++)
			for (int4 x = 0; x < tiles; x++)
				layer->tileAt({ x, y })->clear({ 0.3f, 0.5f, 0.7f, 1.0f });
	}

	TEST(UploadsStayWithinThePixelBudget) {
		AppScene scene;
		TileRenderer& renderer = scene.application.renderer();
		renderer.setUploadTilesPerFrame(1000);
		fillArea(scene.document(), 12);
		scene.driver.frame();
		CHECK(renderer.uploadedPixelsLastFrame() <= TileRenderer::kUploadPixelBudget + (ualni) kTileSize * kTileSize);
		CHECK(renderer.uploadedPixelsLastFrame() > 0);
	}

	TEST(LargeInvalidationsShowTheCoarserLevelFirst) {
		AppScene scene;
		TileRenderer& renderer = scene.application.renderer();
		renderer.setUploadTilesPerFrame(1000);
		fillArea(scene.document(), 12);
		scene.driver.frame();
		CHECK_EQUAL(0, renderer.wantedLevelLastFrame());
		CHECK_EQUAL(1, renderer.mipLevelLastFrame());
		CHECK(renderer.drawnTilesLastFrame() > 0);

		int frames = 0;
		while ((renderer.mipLevelLastFrame() != 0 || renderer.hasPendingUploads()) && frames < 40) {
			scene.driver.frame();
			frames++;
		}
		CHECK_EQUAL(0, renderer.mipLevelLastFrame());
		CHECK(!renderer.hasPendingUploads());
		CHECK(frames >= TileRenderer::kCoarseHoldFrames - 1);
	}

	TEST(TilesNotUploadedYetFallBackToTheirParent) {
		AppScene scene;
		TileRenderer& renderer = scene.application.renderer();
		renderer.setUploadTilesPerFrame(2);
		fillArea(scene.document(), 2);
		scene.driver.frame();
		CHECK_EQUAL(0, renderer.mipLevelLastFrame());
		CHECK(renderer.hasPendingUploads());
		CHECK(renderer.fallbackTilesLastFrame() > 0);
		CHECK_EQUAL(4u, renderer.drawnTilesLastFrame() + renderer.fallbackTilesLastFrame());
	}
}
