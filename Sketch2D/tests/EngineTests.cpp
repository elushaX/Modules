#include "Document.hpp"
#include "StrokePath.hpp"
#include "Compositor.hpp"
#include "Engine.hpp"
#include "LayerLevels.hpp"
#include "Profiler.hpp"

#include "UnitTest++/UnitTest++.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	class RecordingOperation : public Operation {
	public:
		std::vector<int4> levels;
		std::vector<int4> finished;
		std::atomic<int4> running{ 0 };
		int4 sleepMs = 0;
		uint1 value = 200;

		[[nodiscard]] const char* name() const override { return "recording"; }

		std::atomic<int4> maxRunning{ 0 };

		void run(const TileWork& work, Tile& target) override {
			const int4 now = ++running;
			int4 seen = maxRunning.load();
			while (now > seen && !maxRunning.compare_exchange_weak(seen, now)) {
			}
			if (sleepMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
			target.pixels()[3] = value;
			{
				std::lock_guard lock(mutex);
				levels.push_back(work.level);
			}
			running--;
		}

		[[nodiscard]] size_t ran() {
			std::lock_guard lock(mutex);
			return levels.size();
		}

		std::mutex mutex;

		void finish(Layer& layer, const TileWork& work, const std::shared_ptr<Tile>& result) override {
			finished.push_back(work.level);
			Operation::finish(layer, work, result);
		}
	};

	Engine::Job jobAt(Layer& layer, const std::shared_ptr<Operation>& operation, int4 level, int4 x = 0) {
		layer.tileAt({ x, 0 });
		return { operation, layer.sharedTile({ x, 0 }), &layer, { { x, 0 }, level } };
	}

	void fill(Layer& layer, const TileCoord& coord, uint1 r, uint1 g, uint1 b, uint1 a) {
		uint1* pixels = layer.tileAt(coord)->pixels();
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize; index++) {
			pixels[index * 4 + 0] = r;
			pixels[index * 4 + 1] = g;
			pixels[index * 4 + 2] = b;
			pixels[index * 4 + 3] = a;
		}
	}
}

SUITE(Engine) {
	TEST(CoarseLevelsRunBeforeFineOnes) {
		Layer layer(1, "Layer");
		Engine engine;
		auto operation = std::make_shared<RecordingOperation>();
		for (int4 level : { 0, 3, 1, 6, 2 })
			engine.submit(jobAt(layer, operation, level, level));
		CHECK_EQUAL(5u, engine.pending());
		CHECK_EQUAL(1u, engine.pending(3));
		engine.drain();
		REQUIRE CHECK_EQUAL(5u, operation->levels.size());
		CHECK((operation->levels == std::vector<int4>{ 6, 3, 2, 1, 0 }));
		CHECK_EQUAL(0u, engine.pending());
	}

	TEST(FinishRunsOnTheCallerAndMarksTheTileChanged) {
		Layer layer(1, "Layer");
		Engine engine(2);
		auto operation = std::make_shared<RecordingOperation>();
		engine.submit(jobAt(layer, operation, 0));
		layer.clearChanges();
		const ualni before = engine.completed();
		while (engine.completed() == before)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		CHECK(operation->finished.empty());
		CHECK_EQUAL(0u, layer.changedCount());
		engine.apply();
		CHECK_EQUAL(1u, operation->finished.size());
		CHECK_EQUAL(1u, layer.changedCount());
		CHECK(layer.tileIfExists({ 0, 0 })->hasDirty());
	}

	TEST(TimeSliceStopsWhenTheBudgetIsSpent) {
		Layer layer(1, "Layer");
		Engine engine;
		auto operation = std::make_shared<RecordingOperation>();
		operation->sleepMs = 5;
		for (int4 index = 0; index < 20; index++)
			engine.submit(jobAt(layer, operation, 0, index));
		const size_t ran = engine.runFor(12.0);
		CHECK(ran >= 1 && ran <= 4);
		CHECK_EQUAL(20u - ran, engine.pending());
		CHECK_EQUAL(ran, operation->finished.size());
	}

	TEST(WorkersRenderIntoPrivateTilesAndOnlyTheCallerSwapsThemIn) {
		Document document;
		document.setEngineThreads(3);
		for (int4 tile = 0; tile < 16; tile++)
			fill(*document.activeLayer(), { tile, 0 }, 255, 0, 0, 255);
		document.commit("Fill");
		std::vector<const Tile*> live;
		document.activeLayer()->forEachTile([&](const TileCoord&, Tile* tile) { live.push_back(tile); });
		document.whilePaintPaused([&]() {
			(void) document.applyFilter(Document::PixelFilter::Invert);
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			for (const Tile* tile : live)
				CHECK_EQUAL(255, (int) tile->pixels()[0]);
			CHECK_EQUAL(255, (int) document.activeLayer()->tileIfExists({ 5, 0 })->pixels()[0]);
		});
		document.settlePaint();
		CHECK_EQUAL(0, (int) document.activeLayer()->tileIfExists({ 5, 0 })->pixels()[0]);
		for (const Tile* tile : live)
			CHECK(tile != document.activeLayer()->tileIfExists({ 5, 0 }));
	}

	TEST(OneTileIsNeverWorkedOnByTwoThreadsAtOnce) {
		Layer layer(1, "Layer");
		Engine engine(4);
		auto operation = std::make_shared<RecordingOperation>();
		operation->sleepMs = 2;
		for (int4 index = 0; index < 12; index++)
			engine.submit(jobAt(layer, operation, 0, 0));
		CHECK(engine.hasWork(layer, { 0, 0 }));
		engine.drain();
		CHECK_EQUAL(12u, operation->finished.size());
		CHECK(operation->maxRunning <= 1);
		CHECK(!engine.hasWork(layer, { 0, 0 }));
	}

	TEST(DrainFromInsideAPauseDoesNotDeadlock) {
		Layer layer(1, "Layer");
		Engine engine(2);
		auto operation = std::make_shared<RecordingOperation>();
		engine.whilePaused([&]() {
			engine.submit(jobAt(layer, operation, 0));
			engine.drain();
			engine.apply();
		});
		engine.drain();
		CHECK_EQUAL(1u, operation->finished.size());
	}

	TEST(ThreadsCanBeChangedWithWorkQueued) {
		Layer layer(1, "Layer");
		Engine engine;
		auto operation = std::make_shared<RecordingOperation>();
		for (int4 index = 0; index < 8; index++)
			engine.submit(jobAt(layer, operation, 0, index));
		engine.setThreads(4);
		CHECK_EQUAL(4u, engine.threads());
		CHECK_EQUAL(8u, operation->finished.size());
		for (int4 index = 0; index < 8; index++)
			engine.submit(jobAt(layer, operation, 1, index));
		engine.drain();
		CHECK_EQUAL(16u, operation->finished.size());
	}

	TEST(FilterReturnsBeforeThePixelsChangeWithAnEngineThread) {
		Document document;
		document.setEngineThreads(1);
		for (int4 tile = 0; tile < 16; tile++)
			fill(*document.activeLayer(), { tile % 4, tile / 4 }, 255, 0, 0, 255);
		document.commit("Fill");
		document.whilePaintPaused([&]() {
			CHECK(document.applyFilter(Document::PixelFilter::Invert) == Document::FilterResult::Done);
			CHECK_EQUAL(16u, document.engine().pending());
			CHECK_EQUAL(255, (int) document.activeLayer()->tileIfExists({ 0, 0 })->pixels()[0]);
		});
		const RGBA inverted = document.sampleColor({ 3.5f, 3.5f }, false);
		CHECK_CLOSE(0.0f, inverted.r, 0.01f);
		CHECK_CLOSE(1.0f, inverted.g, 0.01f);
		CHECK_EQUAL(0u, document.paintBacklog());
	}

	TEST(FilterKeepsTheUndoSnapshotIntact) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		document.commit("Fill");
		const Tile* original =
			document.history().current().state.layers.front().tiles.get(TileCoord{ 0, 0 }.key()).tile.get();
		CHECK(document.applyFilter(Document::PixelFilter::Invert) == Document::FilterResult::Done);
		document.settlePaint();
		CHECK_EQUAL(255, (int) original->pixels()[0]);
		CHECK_EQUAL(0, (int) document.activeLayer()->tileIfExists({ 0, 0 })->pixels()[0]);
		document.commit("Invert");
		document.undo();
		CHECK_EQUAL(255, (int) document.activeLayer()->tileIfExists({ 0, 0 })->pixels()[0]);
	}

	TEST(SubmittingPaintRunsATimeSliceWithoutThreads) {
		Document document;
		for (int4 tile = 0; tile < 4; tile++)
			fill(*document.activeLayer(), { tile, 0 }, 255, 0, 0, 255);
		document.commit("Fill");
		document.whilePaintPaused([&]() { (void) document.applyFilter(Document::PixelFilter::Invert); });
		CHECK_EQUAL(4u, document.engine().pending());
		document.submitPaint();
		CHECK_EQUAL(0u, document.engine().pending());
		CHECK_EQUAL(0, (int) document.activeLayer()->tileIfExists({ 3, 0 })->pixels()[0]);
	}

	void zoomOut(Document & document) {
		for (int4 tile = 0; tile < 64; tile++)
			fill(*document.activeLayer(), { tile % 8, tile / 8 }, 255, 255, 255, 255);
		document.commit("Fill");
		document.setZoom(1.0f / 8.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 3);
	}

	Brush blackBrush(halnf size) {
		Brush brush;
		brush.setSize(size);
		brush.setHardness(1.0f);
		brush.color = { 0, 0, 0, 1 };
		return brush;
	}

	TEST(ZoomedOutPreviewOfASmallBrushIsAntialiasedLikeTheResult) {
		Document document;
		zoomOut(document);
		document.setZoom(1.0f / 16.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 4);
		Brush brush = blackBrush(20);
		brush.setHardness(0.8f);
		for (int4 dab = 0; dab < 2000; dab++)
			document.stampBrushDab({ 100.0f + (halnf) dab * 0.5f, 503.3f + 3.0f * std::sin((halnf) dab * 0.01f) }, brush);
		CHECK(document.deferredDabs() == 2000u);
		std::vector<int4> preview;
		const Tile* previewTile = document.activeLayer()->levels().tileAt(4, { 0, 0 });
		REQUIRE CHECK(previewTile != nullptr);
		for (int4 y = 26; y < 38; y++)
			for (int4 x = 10; x < 60; x++)
				preview.push_back(255 - previewTile->pixels()[((ualni) y * kTileSize + x) * 4]);
		document.finishStroke();
		document.settlePaint();
		document.activeLayer()->levels().update(*document.activeLayer(), 4);
		const Tile* finalTile = document.activeLayer()->levels().tileAt(4, { 0, 0 });
		int4 worst = 0;
		int4 partial = 0;
		size_t index = 0;
		for (int4 y = 26; y < 38; y++)
			for (int4 x = 10; x < 60; x++) {
				const int4 coverage = preview[index++];
				worst =
					std::max(worst, std::abs(coverage - (255 - (int4) finalTile->pixels()[((ualni) y * kTileSize + x) * 4])));
				partial += coverage > 20 && coverage < 235 ? 1 : 0;
			}
		CHECK(worst < 128);
		CHECK(partial > 40);
	}

	TEST(ZoomedOutPathStrokeEndsWithTheSamePixelsAsAtFullZoom) {
		std::vector<StrokePath::Sample> samples;
		for (int4 index = 0; index <= 40; index++)
			samples.push_back({ { 100.0f + (halnf) index * 9.0f, 200.0f + std::sin((halnf) index * 0.4f) * 60.0f }, 1.0f });
		Brush brush = blackBrush(20);
		brush.setSpacing(0.05f);
		std::vector<halnf> reference;
		for (const halnf zoom : { 1.0f, 1.0f / 16.0f }) {
			Document document;
			document.setZoom(zoom);
			document.activeLayer()->levels().update(*document.activeLayer(), LayerLevels::levelForZoom(zoom));
			REQUIRE CHECK_EQUAL(LayerLevels::levelForZoom(zoom), document.activeLayer()->levels().builtLevels());
			document.beginPathStroke(brush, {}, samples.front());
			for (size_t index = 1; index < samples.size(); index++) {
				document.addPathSample(samples[index]);
				document.advancePathStroke(false);
			}
			document.finishStroke();
			document.settlePaint();
			std::vector<halnf> alpha;
			for (int4 y = 120; y < 280; y += 3)
				for (int4 x = 90; x < 470; x += 3)
					alpha.push_back(document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false).a);
			if (reference.empty()) {
				reference = alpha;
				continue;
			}
			int4 painted = 0;
			halnf worst = 0;
			for (size_t index = 0; index < alpha.size(); index++) {
				painted += reference[index] > 0.5f ? 1 : 0;
				worst = std::max(worst, std::abs(reference[index] - alpha[index]));
			}
			CHECK(painted > 200);
			CHECK(worst < 0.02f);
		}
	}

	TEST(ZoomingDuringAPathStrokeKeepsTheSameDabs) {
		std::vector<StrokePath::Sample> samples;
		for (int4 index = 0; index <= 60; index++)
			samples.push_back({ { 100.0f + (halnf) index * 6.0f, 200.0f + std::cos((halnf) index * 0.3f) * 50.0f }, 1.0f });
		Brush brush = blackBrush(16);
		brush.setSpacing(0.05f);
		brush.setFlow(0.3f);
		std::vector<halnf> reference;
		for (const bool switching : { false, true }) {
			Document document;
			document.activeLayer()->levels().update(*document.activeLayer(), 4);
			document.setZoom(1.0f);
			document.beginPathStroke(brush, {}, samples.front());
			for (size_t index = 1; index < samples.size(); index++) {
				if (switching) document.setZoom(index % 20 < 10 ? 1.0f / 16.0f : 1.0f);
				document.addPathSample(samples[index]);
				document.advancePathStroke(false);
			}
			document.finishStroke();
			document.settlePaint();
			std::vector<halnf> alpha;
			for (int4 y = 130; y < 270; y += 2)
				for (int4 x = 90; x < 480; x += 2)
					alpha.push_back(document.sampleColor({ (halnf) x + 0.5f, (halnf) y + 0.5f }, false).a);
			if (reference.empty()) {
				reference = alpha;
				continue;
			}
			halnf worst = 0;
			for (size_t index = 0; index < alpha.size(); index++)
				worst = std::max(worst, std::abs(reference[index] - alpha[index]));
			CHECK(worst < 0.02f);
		}
	}

	TEST(ZoomedOutStrokesPaintTheVisibleLevelFirst) {
		Document document;
		zoomOut(document);
		const Brush brush = blackBrush(200);
		document.stampBrushDab({ 500, 500 }, brush);
		CHECK_EQUAL(3, document.previewLevel());
		CHECK(document.deferredDabs() > 0);
		const LayerLevels& levels = document.activeLayer()->levels();
		CHECK(levels.isPreview(3, { 0, 0 }));
		const uint1* coarse = levels.tileAt(3, { 0, 0 })->pixels() + ((ualni) 62 * kTileSize + 62) * 4;
		CHECK_EQUAL(0, (int) coarse[0]);
		CHECK_EQUAL(255, (int) coarse[3]);
		CHECK_EQUAL(
			255, (int) document.activeLayer()->tileIfExists({ 3, 3 })->pixels()[((ualni) 116 * kTileSize + 116) * 4]
		);
	}

	TEST(FinishingAZoomedOutStrokeReturnsBeforeFullResolution) {
		Document document;
		zoomOut(document);
		document.setEngineThreads(1);
		const Brush brush = blackBrush(200);
		const size_t revision = document.history().revision();
		document.whilePaintPaused([&]() {
			document.stampBrushDab({ 500, 500 }, brush);
			document.finishStroke();
			CHECK_EQUAL(revision + 1, document.history().revision());
			CHECK(document.engine().pending() > 0);
			CHECK_EQUAL(0u, document.deferredDabs());
		});
		const RGBA painted = document.sampleColor({ 500.5f, 500.5f }, false);
		CHECK_CLOSE(0.0f, painted.r, 0.01f);
		CHECK_CLOSE(1.0f, painted.a, 0.01f);
		CHECK_EQUAL(0u, document.activeLayer()->levels().previewCount());
	}

	TEST(ZoomedOutStrokeMatchesTheEagerFullResolutionResult) {
		Document lazy;
		Document eager;
		zoomOut(lazy);
		zoomOut(eager);
		eager.setZoom(1.0f);
		const Brush brush = blackBrush(150);
		for (Document* document : { &lazy, &eager }) {
			for (int4 dab = 0; dab < 5; dab++)
				document->stampBrushDab({ 300.0f + (halnf) dab * 40.0f, 410.0f }, brush);
			document->finishStroke();
			document->settlePaint();
		}
		bool same = true;
		eager.activeLayer()->forEachTile([&](const TileCoord& coord, Tile* tile) {
			const Tile* other = lazy.activeLayer()->tileIfExists(coord);
			same = same && other && std::memcmp(tile->pixels(), other->pixels(), (size_t) kTileSize * kTileSize * 4) == 0;
		});
		CHECK(same);
		CHECK_EQUAL(eager.tileCount(), lazy.tileCount());
	}

	TEST(UndoAfterAZoomedOutStrokeRestoresThePixels) {
		Document document;
		zoomOut(document);
		const Brush brush = blackBrush(200);
		document.stampBrushDab({ 500, 500 }, brush);
		document.finishStroke();
		CHECK_CLOSE(0.0f, document.sampleColor({ 500.5f, 500.5f }, false).r, 0.01f);
		document.undo();
		CHECK_CLOSE(1.0f, document.sampleColor({ 500.5f, 500.5f }, false).r, 0.01f);
		document.redo();
		CHECK_CLOSE(0.0f, document.sampleColor({ 500.5f, 500.5f }, false).r, 0.01f);
	}

	TEST(PreviewTilesAreReplacedByTheRealDownsampleAfterwards) {
		Document document;
		zoomOut(document);
		const Brush brush = blackBrush(200);
		document.stampBrushDab({ 500, 500 }, brush);
		document.finishStroke();
		document.settlePaint();
		LayerLevels& levels = document.activeLayer()->levels();
		CHECK(!levels.isPreview(3, { 0, 0 }));
		levels.update(*document.activeLayer(), 3);
		CHECK(levels.lastDownsampledTiles() > 0);
		const uint1* coarse = levels.tileAt(3, { 0, 0 })->pixels() + ((ualni) 62 * kTileSize + 62) * 4;
		CHECK_EQUAL(0, (int) coarse[0]);
	}

	TEST(FinishingAZoomedInStrokeReturnsBeforeFullResolution) {
		Document document;
		document.setZoom(1.0f);
		document.setEngineThreads(1);
		const Brush brush = blackBrush(300);
		const size_t revision = document.history().revision();
		document.whilePaintPaused([&]() {
			for (int4 dab = 0; dab < 20; dab++)
				document.stampBrushDab({ 200.0f + (halnf) dab * 30.0f, 400.0f }, brush);
			document.finishStroke();
			CHECK_EQUAL(revision + 1, document.history().revision());
			CHECK(document.engine().pending() > 0);
			CHECK(document.activeLayer()->tileIfExists({ 3, 3 }) == nullptr);
			CHECK(document.activeLayer()->levels().isPreview(0, { 3, 3 }));
		});
		CHECK_CLOSE(1.0f, document.sampleColor({ 500.5f, 400.5f }, false).a, 0.01f);
		CHECK_EQUAL(0u, document.activeLayer()->levels().previewCount());
		document.undo();
		CHECK_CLOSE(0.0f, document.sampleColor({ 500.5f, 400.5f }, false).a, 0.01f);
		document.redo();
		CHECK_CLOSE(1.0f, document.sampleColor({ 500.5f, 400.5f }, false).a, 0.01f);
	}

	TEST(ZoomedInStrokesPaintTheBasePreviewAndDeferTheLayer) {
		Document document;
		zoomOut(document);
		document.setZoom(1.0f);
		document.stampBrushDab({ 500, 500 }, blackBrush(40));
		CHECK(document.deferredDabs() > 0);
		CHECK_EQUAL(0, document.previewLevel());
		CHECK(document.activeLayer()->levels().isPreview(0, { 3, 3 }));
		document.finishStroke();
		CHECK_CLOSE(0.0f, document.sampleColor({ 500.5f, 500.5f }, false).r, 0.01f);
	}

	TEST(ZoomedOutDabsCostLevelPixelsNotFullResolution) {
		Document document;
		zoomOut(document);
		Profiler& profiler = Profiler::global();
		profiler.setEnabled(true);
		profiler.clear();
		profiler.beginFrame();
		document.stampBrushDab({ 500, 500 }, blackBrush(800));
		profiler.endFrame();
		const double preview = profiler.counterTotal("brush.preview_pixels");
		profiler.clear();
		profiler.setEnabled(false);
		CHECK(preview > 0);
		CHECK(preview <= (double) kTileSize * kTileSize * 4);
		document.settlePaint();
	}

	TEST(PreviewTilesSurviveLevelUpdatesWhileTheStrokeIsPending) {
		Document document;
		zoomOut(document);
		document.stampBrushDab({ 500, 500 }, blackBrush(200));
		LayerLevels& levels = document.activeLayer()->levels();
		fill(*document.activeLayer(), { 0, 0 }, 255, 255, 255, 255);
		levels.update(*document.activeLayer(), 3);
		const uint1* coarse = levels.tileAt(3, { 0, 0 })->pixels() + ((ualni) 62 * kTileSize + 62) * 4;
		CHECK_EQUAL(0, (int) coarse[0]);
		document.settlePaint();
	}

	TEST(EndingAPreviewRebuildsItFromTheFinerLevel) {
		Document document;
		zoomOut(document);
		LayerLevels& levels = document.activeLayer()->levels();
		Tile* preview = levels.previewTile(3, { 0, 0 });
		REQUIRE CHECK(preview != nullptr);
		preview->pixels()[0] = 7;
		levels.endPreview();
		levels.update(*document.activeLayer(), 3);
		CHECK_EQUAL(255, (int) levels.tileAt(3, { 0, 0 })->pixels()[0]);
	}

	TEST(FinishingAZoomedOutStrokeDoesNotWaitForTheEngine) {
		Document document;
		zoomOut(document);
		const Brush brush = blackBrush(600);
		for (int4 dab = 0; dab < 4; dab++)
			document.stampBrushDab({ 300.0f + (halnf) dab * 150.0f, 500.0f }, brush);
		document.finishStroke();
		CHECK(document.engine().pending() > 0);
		document.settlePaint();
		CHECK_EQUAL(0u, document.engine().pending());
	}

	TEST(UndoAndRedoWalkSeveralDeferredStrokesWithAnEngineThread) {
		Document document;
		zoomOut(document);
		document.setEngineThreads(2);
		const Brush brush = blackBrush(200);
		document.stampBrushDab({ 300, 300 }, brush);
		document.finishStroke();
		document.stampBrushDab({ 700, 700 }, brush);
		document.finishStroke();
		document.settlePaint();
		CHECK_CLOSE(0.0f, document.sampleColor({ 300.5f, 300.5f }, false).r, 0.01f);
		CHECK_CLOSE(0.0f, document.sampleColor({ 700.5f, 700.5f }, false).r, 0.01f);
		document.undo();
		CHECK_CLOSE(0.0f, document.sampleColor({ 300.5f, 300.5f }, false).r, 0.01f);
		CHECK_CLOSE(1.0f, document.sampleColor({ 700.5f, 700.5f }, false).r, 0.01f);
		document.undo();
		CHECK_CLOSE(1.0f, document.sampleColor({ 300.5f, 300.5f }, false).r, 0.01f);
		document.redo();
		document.redo();
		CHECK_CLOSE(0.0f, document.sampleColor({ 700.5f, 700.5f }, false).r, 0.01f);
	}

	TEST(HistoryHoldsTheFinishedPixelsOfADeferredStroke) {
		Document document;
		zoomOut(document);
		document.stampBrushDab({ 500, 500 }, blackBrush(200));
		document.finishStroke();
		document.settlePaint();
		const auto& tiles = document.history().current().state.layers.front().tiles;
		const Tile* stored = tiles.get(TileCoord{ 3, 3 }.key()).tile.get();
		CHECK(stored == document.activeLayer()->tileIfExists({ 3, 3 }));
		CHECK_EQUAL(0, (int) stored->pixels()[((ualni) 116 * kTileSize + 116) * 4]);
	}

	TEST(ErasingADeferredStrokeToNothingDropsTheTile) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 255, 255, 255);
		document.commit("Fill");
		document.setZoom(1.0f / 8.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 3);
		Brush eraser = blackBrush(600);
		eraser.setEraserMode(true);
		document.stampBrushDab({ 64, 64 }, eraser);
		document.finishStroke();
		document.settlePaint();
		CHECK(document.activeLayer()->tileIfExists({ 0, 0 }) == nullptr);
		CHECK(document.history().current().state.layers.front().tiles.size() == 0);
		document.undo();
		CHECK(document.activeLayer()->tileIfExists({ 0, 0 }) != nullptr);
	}

	TEST(TwoDeferredStrokesOnTheSameTilesBothLand) {
		for (size_t threads : { (size_t) 0, (size_t) 2 }) {
			Document document;
			zoomOut(document);
			document.setEngineThreads(threads);
			Brush red = blackBrush(300);
			red.color = { 1, 0, 0, 1 };
			Brush green = blackBrush(300);
			green.color = { 0, 1, 0, 1 };
			for (int4 dab = 0; dab < 6; dab++)
				document.stampBrushDab({ 100.0f + (halnf) dab * 100.0f, 500.0f }, red);
			document.finishStroke();
			for (int4 dab = 0; dab < 6; dab++)
				document.stampBrushDab({ 500.0f, 100.0f + (halnf) dab * 100.0f }, green);
			document.finishStroke();
			document.settlePaint();
			const RGBA left = document.sampleColor({ 150.5f, 500.5f }, false);
			const RGBA cross = document.sampleColor({ 500.5f, 500.5f }, false);
			CHECK(left.r > 0.9f);
			CHECK(cross.g > 0.9f);
			CHECK(cross.r < 0.1f);
			document.undo();
			CHECK(document.sampleColor({ 150.5f, 500.5f }, false).r > 0.9f);
			CHECK(document.sampleColor({ 500.5f, 500.5f }, false).r > 0.9f);
		}
	}

	TEST(DeferredEraserStrokeAfterADeferredBrushStrokeErases) {
		Document document;
		zoomOut(document);
		document.setEngineThreads(2);
		Brush red = blackBrush(300);
		red.color = { 1, 0, 0, 1 };
		document.stampBrushDab({ 500, 500 }, red);
		document.finishStroke();
		Brush eraser = blackBrush(600);
		eraser.setEraserMode(true);
		document.stampBrushDab({ 500, 500 }, eraser);
		document.finishStroke();
		document.settlePaint();
		CHECK(document.sampleColor({ 500.5f, 500.5f }, false).a < 0.02f);
	}

	TEST(EagerEraserAfterEagerStrokeWithPaintWorkers) {
		Document document;
		document.setPaintWorkers(4);
		document.setZoom(0.25f);
		Brush red = blackBrush(160);
		red.color = { 1, 0, 0, 1 };
		for (int4 dab = 0; dab < 40; dab++)
			document.stampBrushDab({ -1600.0f + (halnf) dab * 80.0f, 0.0f }, red);
		document.finishStroke();
		Brush eraser = blackBrush(480);
		eraser.setEraserMode(true);
		for (int4 dab = 0; dab < 40; dab++)
			document.stampBrushDab({ -1600.0f + (halnf) dab * 80.0f, 0.0f }, eraser);
		document.finishStroke();
		document.settlePaint();
		const RGBA left = document.sampleColor({ 0.5f, 0.5f }, false);
		std::printf("[engine] eager eraser alpha %.3f\n", left.a);
		CHECK(left.a < 0.02f);
	}

	TEST(SettlingInsideAPaintPauseWithStagedDabsDoesNotDeadlock) {
		Document document;
		document.setPaintWorkers(4);
		document.setEngineThreads(2);
		Brush brush = blackBrush(200);
		for (int4 dab = 0; dab < 30; dab++)
			document.stampBrushDab({ (halnf) dab * 30.0f, 100.0f }, brush);
		document.submitPaint();
		for (int4 dab = 0; dab < 30; dab++)
			document.stampBrushDab({ (halnf) dab * 30.0f, 300.0f }, brush);
		std::atomic<bool> done{ false };
		std::thread watchdog([&]() {
			for (int4 wait = 0; wait < 100 && !done.load(); wait++)
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			if (!done.load()) {
				std::fprintf(stderr, "deadlock: settlePaint inside whilePaintPaused\n");
				std::_Exit(3);
			}
		});
		document.whilePaintPaused([&]() { document.settlePaint(); });
		done.store(true);
		watchdog.join();
		document.finishStroke();
		CHECK(document.sampleColor({ 450.5f, 100.5f }, false).a > 0.9f);
		CHECK(document.sampleColor({ 450.5f, 300.5f }, false).a > 0.9f);
		CHECK_EQUAL(0u, document.paintBacklog());
	}

	TEST(ZoomedOutStrokeShowsItsOpacityWhileStillBeingDrawn) {
		Document document;
		document.setZoom(1.0f / 8.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 3);
		Brush brush = blackBrush(400);
		brush.setOpacity(0.5f);
		for (int4 dab = 0; dab < 40; dab++)
			document.stampBrushDab({ 400.0f + (halnf) (dab % 4) * 10.0f, 400.0f }, brush);
		REQUIRE CHECK(document.deferredDabs() > 0);
		const Tile* preview = document.activeLayer()->levels().tileAt(3, { 0, 0 });
		REQUIRE CHECK(preview != nullptr);
		const int4 alpha = preview->pixels()[((ualni) 50 * kTileSize + 50) * 4 + 3];
		CHECK_CLOSE(128, alpha, 3);
		document.finishStroke();
		document.settlePaint();
		CHECK_CLOSE(0.5f, document.sampleColor({ 400.5f, 400.5f }, false).a, 0.02f);
	}

	TEST(LevelTileWaitsForAllFourChildrenBeforeRebuilding) {
		Document document;
		Layer& layer = *document.activeLayer();
		for (int4 index = 0; index < 4; index++)
			fill(layer, { index & 1, index >> 1 }, 0, 0, 0, 255);
		LayerLevels& levels = layer.levels();
		const auto pending = std::make_shared<Map<ualni, bool>>();
		levels.setPendingCheck([pending](const TileCoord& coord) { return pending->presents(coord.key()).isValid(); });
		levels.update(layer, 1);
		const ualni built = levels.rebuildCount();
		REQUIRE CHECK(levels.tileAt(1, { 0, 0 }) != nullptr);
		CHECK_EQUAL(255, levels.tileAt(1, { 0, 0 })->pixels()[3]);

		for (int4 index = 0; index < 4; index++)
			pending->put(TileCoord{ index & 1, index >> 1 }.key(), true);
		for (int4 index = 0; index < 4; index++) {
			const TileCoord child = { index & 1, index >> 1 };
			fill(layer, child, 0, 0, 0, 0);
			layer.noteChanged(child);
			pending->remove(child.key());
			levels.update(layer, 1);
			if (index < 3) {
				CHECK_EQUAL(built, levels.rebuildCount());
				CHECK(levels.isStale(1, { 0, 0 }));
				CHECK_EQUAL(255, levels.tileAt(1, { 0, 0 })->pixels()[3]);
			}
		}
		CHECK_EQUAL(built + 1, levels.rebuildCount());
		CHECK(!levels.isStale(1, { 0, 0 }));
		CHECK(levels.tileAt(1, { 0, 0 }) == nullptr || levels.tileAt(1, { 0, 0 })->pixels()[3] == 0);
	}

	TEST(LevelPyramidAfterSettleMatchesAFreshBuild) {
		Document document;
		document.setEngineThreads(2);
		document.setZoom(1.0f / 8.0f);
		Brush brush;
		brush.setSize(500);
		for (int4 dab = 0; dab < 12; dab++)
			document.stampBrushDab({ (halnf) dab * 150.0f, 300.0f + (halnf) (dab % 3) * 80.0f }, brush);
		document.finishStroke();
		Layer& layer = *document.activeLayer();
		Compositor compositor;
		for (int4 frame = 0; frame < 6; frame++) {
			document.whilePaintPaused([&]() { compositor.update(document, 3); });
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		document.settlePaint();
		document.whilePaintPaused([&]() { compositor.update(document, 3); });
		CHECK_EQUAL(0u, layer.levels().staleCount());

		LayerLevels fresh;
		fresh.update(layer, 3);
		for (int4 level = 1; level <= 3; level++) {
			layer.levels().forEachTile(level, [&](const TileCoord& coord, Tile* tile) {
				const Tile* expected = fresh.tileAt(level, coord);
				REQUIRE CHECK(expected != nullptr);
				CHECK(std::memcmp(tile->pixels(), expected->pixels(), (size_t) kTileSize * kTileSize * 4) == 0);
			});
			CHECK_EQUAL(fresh.tileCount(level), layer.levels().tileCount(level));
		}
	}

	TEST(EngineWorkStaysPendingUntilItsResultIsApplied) {
		Document document;
		Layer& layer = *document.activeLayer();
		Engine engine(1);
		auto operation = std::make_shared<RecordingOperation>();
		engine.submit(jobAt(layer, operation, 0));
		for (int4 wait = 0; wait < 200 && operation->ran() == 0; wait++)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		CHECK(engine.hasWork(layer, { 0, 0 }));
		engine.apply();
		CHECK(!engine.hasWork(layer, { 0, 0 }));
	}
}
