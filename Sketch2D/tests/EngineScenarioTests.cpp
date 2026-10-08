#include "Compositor.hpp"
#include "Document.hpp"
#include "LayerThumbnail.hpp"
#include "Profiler.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>
#include <thread>
#include <cstdio>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	struct Measure {
		double ms = 0;
		double dabPixels = 0;

		template <typename Fn>
		static Measure of(Fn&& fn) {
			Profiler& profiler = Profiler::global();
			const bool wasEnabled = profiler.isEnabled();
			profiler.setEnabled(true);
			profiler.clear();
			profiler.beginFrame();
			const auto start = std::chrono::steady_clock::now();
			fn();
			const auto end = std::chrono::steady_clock::now();
			profiler.endFrame();
			Measure measure;
			measure.ms = std::chrono::duration<double, std::milli>(end - start).count();
			measure.dabPixels = profiler.counterTotal("brush.pixels_touched");
			profiler.clear();
			profiler.setEnabled(wasEnabled);
			return measure;
		}
	};

	void report(const char* scenario, const Measure& measure) {
		std::printf("[engine-scenario] %-34s %9.2f ms %12.0f dab px\n", scenario, measure.ms, measure.dabPixels);
	}

	void fillLayer(Document& document, int4 size) {
		Layer* layer = document.activeLayer();
		for (int4 ty = 0; ty < size / kTileSize; ty++) {
			for (int4 tx = 0; tx < size / kTileSize; tx++)
				layer->tileAt({ tx, ty })->clear({ 0.2f, 0.4f, 0.6f, 1.0f });
		}
		document.commit("Fill");
	}

	Brush bigBrush(halnf size) {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Round));
		brush.setSize(size);
		return brush;
	}
}

SUITE(BenchEngineScenarios) {
	TEST(TilesAre128PixelsLikeTheReference) { CHECK_EQUAL(128, kTileSize); }

	TEST(BigBrushZoomedOutDab) {
		Document document;
		fillLayer(document, 1024);
		document.setZoom(1.0f / 16.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), LayerLevels::levelForZoom(document.zoom()));
		const Brush brush = bigBrush(1500);
		const Measure stroke = Measure::of([&]() {
			for (int4 dab = 0; dab < 8; dab++)
				document.stampBrushDab({ 2000.0f + (halnf) dab * 100.0f, 2000.0f }, brush, 1.0f);
		});
		report("8 dabs 1500px at zoom 1/16", stroke);
		const Measure finish = Measure::of([&]() { document.finishStroke("Paint"); });
		report("finish that stroke", finish);
		CHECK(finish.dabPixels > 0);
	}

	TEST(TransformDragOnLargeLayer) {
		Document document;
		fillLayer(document, 2048);
		document.setZoom(1.0f / 8.0f);
		REQUIRE CHECK(document.beginTransform());
		const Measure measure = Measure::of([&]() {
			LayerTransform transform;
			transform.pivot = { 1024, 1024 };
			transform.offset = { 37, 11 };
			transform.rotation = 0.1f;
			document.previewTransform(transform);
		});
		report("transform move 2048 layer, zoom 1/8", measure);
		document.cancelTransform();
		CHECK(measure.ms >= 0);
	}

	TEST(BucketOnLargeLayer) {
		Document document;
		fillLayer(document, 2048);
		const Measure measure = Measure::of([&]() {
			BucketSettings settings;
			settings.allLayers = false;
			(void) document.bucketFill({ 100, 100 }, { 1, 0, 0, 1 }, settings);
		});
		report("bucket 2048 layer", measure);
		CHECK(measure.ms >= 0);
	}

	TEST(FilterOnLargeLayer) {
		Document document;
		fillLayer(document, 4096);
		const Measure measure = Measure::of([&]() { (void) document.applyFilter(Document::PixelFilter::Invert); });
		report("invert 4096 layer", measure);
		CHECK(measure.ms >= 0);
	}

	TEST(ThumbnailAfterAHugeZoomedOutStroke) {
		Document document;
		document.setZoom(1.0f / 32.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 5);
		const Brush brush = bigBrush(3000);
		for (int4 dab = 0; dab < 40; dab++)
			document.stampBrushDab({ (halnf) dab * 600.0f, (halnf) dab * 300.0f }, brush, 1.0f);
		const Measure finish = Measure::of([&]() { document.finishStroke("Paint"); });
		report("finish huge stroke @1/32", finish);
		std::vector<uint1> pixels;
		const Measure thumbnail = Measure::of([&]() {
			document.whilePaintPaused([&]() {
				pixels = LayerThumbnail::render(*document.activeLayer(), LayerThumbnail::documentBounds(document));
			});
		});
		report("layer thumbnail right after", thumbnail);
		const Measure settle = Measure::of([&]() { document.settlePaint(); });
		report("settle the full resolution", settle);
		CHECK(!pixels.empty());
	}

	TEST(CompositeCostWhilePaintingZoomedOut) {
		Document document;
		document.setZoom(1.0f / 32.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 5);
		Compositor compositor;
		const Brush brush = bigBrush(700);
		for (int4 dab = 0; dab < 200; dab++) {
			document.stampBrushDab({ (halnf) dab * 60.0f, (halnf) dab * 40.0f }, brush, 1.0f);
			if (dab % 10 == 9) document.finishStroke("Paint");
		}
		document.settlePaint();
		compositor.update(document, 5);
		const Measure stroke = Measure::of([&]() {
			for (int4 dab = 0; dab < 20; dab++) {
				document.stampBrushDab({ 6000.0f + (halnf) dab * 60.0f, 4000.0f }, brush, 1.0f);
				document.whilePaintPaused([&]() {
					compositor.update(document, 5);
					compositor.update(document, 6);
				});
			}
		});
		report("20 dabs + composite each @1/32", stroke);
		std::printf(
			"[engine-scenario] last composited tiles %d pixels %d\n",
			(int) compositor.lastCompositedTiles(),
			(int) compositor.lastCompositedPixels()
		);
		document.finishStroke("Paint");
		CHECK(true);
	}

	TEST(CompositeWhileTheEngineLandsAZoomedOutStroke) {
		Document document;
		document.setEngineThreads(4);
		document.setZoom(1.0f / 32.0f);
		document.activeLayer()->levels().update(*document.activeLayer(), 5);
		Compositor compositor;
		const Brush brush = bigBrush(700);
		for (int4 dab = 0; dab < 300; dab++)
			document.stampBrushDab({ (halnf) dab * 40.0f, (halnf) dab * 20.0f }, brush, 1.0f);
		document.finishStroke("Paint");
		double worst = 0;
		double total = 0;
		double applyMs = 0;
		double downsampled = 0;
		double level6Ms = 0;
		double level5Ms = 0;
		int4 frames = 0;
		while (document.paintBacklog() > 0 && frames < 2000) {
			const auto start = std::chrono::steady_clock::now();
			document.submitPaint();
			const auto applied = std::chrono::steady_clock::now();
			document.whilePaintPaused([&]() {
				const auto t0 = std::chrono::steady_clock::now();
				compositor.update(document, 6);
				downsampled += (double) document.activeLayer()->levels().lastDownsampledPixels();
				const auto t1 = std::chrono::steady_clock::now();
				compositor.update(document, 5);
				const auto t2 = std::chrono::steady_clock::now();
				level6Ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
				level5Ms += std::chrono::duration<double, std::milli>(t2 - t1).count();
			});
			applyMs += std::chrono::duration<double, std::milli>(applied - start).count();
			const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			worst = std::max(worst, ms);
			total += ms;
			frames++;
			std::this_thread::sleep_for(std::chrono::milliseconds(4));
		}
		std::printf(
			"[engine-scenario] landing frames %d worst %.2f ms mean %.2f ms apply %.2f ms downsampled px/frame %.0f L6 %.2f "
			"L5 %.2f\n",
			frames,
			worst,
			total / std::max(frames, 1),
			applyMs / std::max(frames, 1),
			downsampled / std::max(frames, 1),
			level6Ms / std::max(frames, 1),
			level5Ms / std::max(frames, 1)
		);
		CHECK(frames > 0);
	}
}
