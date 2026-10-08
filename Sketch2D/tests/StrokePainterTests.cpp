#include "Compositor.hpp"
#include "Document.hpp"
#include "DabMask.hpp"
#include "StrokePainter.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	const uint1* pixelAt(const Document& document, int4 x, int4 y) {
		document.settlePaint();
		static const uint1 transparent[4] = {};
		const Tile* tile = document.activeLayer()->tileIfExists({ floordiv(x, kTileSize), floordiv(y, kTileSize) });
		if (!tile) return transparent;
		const int4 lx = x - floordiv(x, kTileSize) * kTileSize;
		const int4 ly = y - floordiv(y, kTileSize) * kTileSize;
		return tile->pixels() + ((ualni) ly * kTileSize + lx) * 4;
	}

	bool sameLayers(const Document& left, const Document& right) {
		left.settlePaint();
		right.settlePaint();
		bool same = left.activeLayer()->tileCount() == right.activeLayer()->tileCount();
		left.activeLayer()->forEachTile([&](const TileCoord& coord, Tile* tile) {
			const Tile* other = right.activeLayer()->tileIfExists(coord);
			same = same && other && std::memcmp(tile->pixels(), other->pixels(), (size_t) kTileSize * kTileSize * 4) == 0;
		});
		return same;
	}

	void scribble(Document& document, const Brush& brush) {
		for (int4 step = 0; step < 400; step++) {
			const halnf t = (halnf) step * 0.05f;
			const Vec2F at = { 300.0f * std::sin(t * 0.7f) + 3.0f * t, 200.0f * std::cos(t * 1.3f) };
			document.stampBrushDab(at, brush, 0.4f + 0.6f * std::fabs(std::sin(t)));
		}
	}

	Brush texturedBrush() {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Jagged));
		brush.setSize(70);
		brush.color = { 0.2f, 0.6f, 0.9f, 1 };
		return brush;
	}

	Brush roundBrush() {
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Round));
		brush.setSize(41);
		brush.set(BrushParameter::Angle, 30);
		brush.set(BrushParameter::Roundness, 0.6f);
		brush.setHardness(0.4f);
		brush.color = { 0.9f, 0.3f, 0.1f, 1 };
		return brush;
	}

	uint1 reference(halnf value) { return (uint1) std::clamp(std::lround(value), 0l, 255l); }

	void referenceStamp(std::vector<uint1>& pixels, int4 side, Vec2F center, const Brush& brush, const DabShape& dab) {
		const halnf source[3] = { brush.color.r * 255.0f, brush.color.g * 255.0f, brush.color.b * 255.0f };
		for (int4 y = 0; y < side; y++)
			for (int4 x = 0; x < side; x++) {
				halnf distance = 0;
				halnf a = dab.coverage((halnf) x + 0.5f - center.x, (halnf) y + 0.5f - center.y, &distance);
				if (a <= 0.0f) continue;
				a = dab.textured(a, { (halnf) x, (halnf) y }, distance) * dab.flow * dab.opacity;
				if (a <= 0.0f) continue;
				uint1* dst = pixels.data() + ((size_t) y * side + x) * 4;
				for (int4 channel = 0; channel < 3; channel++)
					dst[channel] = reference(source[channel] * a + dst[channel] * (1.0f - a));
				dst[3] = reference(255.0f * a + dst[3] * (1.0f - a));
			}
	}
}

SUITE(PreparedDab) {
	TEST(MatchesTheExactShapeEverywhere) {
		DabShape shape;
		shape.size = 57;
		shape.opacity = 1;
		shape.flow = 1;
		shape.interior = 0.7f;
		shape.hardness = 0.35f;
		shape.roundness = 0.45f;
		shape.angleDegrees = 33;
		const PreparedDab prepared(shape);

		halnf worst = 0;
		for (halnf y = -30; y <= 30; y += 0.37f)
			for (halnf x = -30; x <= 30; x += 0.41f) {
				halnf distance = 0;
				halnf preparedDistance = 0;
				const halnf exact = shape.coverage(x, y, &distance);
				const halnf fast = prepared.coverage(x, y, preparedDistance);
				worst = std::max(worst, std::fabs(exact - fast));
			}
		CHECK(worst < 2.0e-5f);
	}

	TEST(RowSpanCoversEveryPaintedPixel) {
		DabShape shape;
		shape.size = 64;
		shape.roundness = 0.3f;
		shape.angleDegrees = 70;
		shape.interior = 1;
		const PreparedDab prepared(shape);

		int4 misses = 0;
		for (halnf y = -33; y <= 33; y += 1) {
			halnf from = 0;
			halnf to = 0;
			const bool any = prepared.rowSpan(y, from, to);
			for (halnf x = -40; x <= 40; x += 0.25f) {
				if (shape.coverage(x, y) <= 0.0f) continue;
				if (!any || x < from - 1e-3f || x > to + 1e-3f) misses++;
			}
		}
		CHECK_EQUAL(0, misses);
	}

	TEST(EmptyShapeIsSkipped) {
		DabShape shape;
		CHECK(PreparedDab(shape).isEmpty());
		shape.size = 4;
		CHECK(!PreparedDab(shape).isEmpty());
	}
}

SUITE(StrokePainter) {
	TEST(PaintsLikeThePerPixelReference) {
		const Brush brush = roundBrush();
		Document document;
		std::vector<uint1> expected((size_t) kTileSize * kTileSize * 4, 0);
		const Vec2F centers[] = { { 60.3f, 70.7f }, { 75.1f, 72.2f }, { 90.6f, 69.4f }, { 128, 128 } };
		for (const Vec2F& center : centers) {
			document.stampBrushDab(center, brush, 0.8f);
			const DabShape shape = DabQuantizer::shape(brush.dabAt(0.8f));
			referenceStamp(expected, kTileSize, DabPlacement::snap(center, shape.size).center, brush, shape);
		}
		document.settlePaint();

		const uint1* painted = document.activeLayer()->tileIfExists({ 0, 0 })->pixels();
		int4 worst = 0;
		for (size_t index = 0; index < expected.size(); index++)
			worst = std::max(worst, std::abs((int4) painted[index] - (int4) expected[index]));
		CHECK(worst <= 1);
	}

	TEST(WorkersPaintExactlyWhatTheMainThreadPaints) {
		for (const Brush& brush : { roundBrush(), texturedBrush() }) {
			Document single;
			Document threaded;
			threaded.setPaintWorkers(4);
			CHECK_EQUAL(4u, threaded.paintWorkers());

			scribble(single, brush);
			scribble(threaded, brush);
			threaded.finishStroke();
			single.finishStroke();
			CHECK(sameLayers(single, threaded));
		}
	}

	TEST(MarkerEraserAndAlphaLockMatchOnWorkers) {
		Document single;
		Document threaded;
		threaded.setPaintWorkers(3);
		for (Document* document : { &single, &threaded }) {
			scribble(*document, roundBrush());
			document->finishStroke();

			Brush marker = roundBrush();
			marker.setMarkerBlend(true);
			marker.setOpacity(0.5f);
			marker.color = { 0, 1, 0, 1 };
			scribble(*document, marker);
			document->finishStroke();

			Brush eraser = roundBrush();
			eraser.setEraserMode(true);
			eraser.setSize(15);
			scribble(*document, eraser);
			document->finishStroke();

			document->activeLayer()->setAlphaLocked(true);
			Brush tint = texturedBrush();
			tint.color = { 1, 0, 1, 1 };
			scribble(*document, tint);
			document->finishStroke();
		}
		CHECK(sameLayers(single, threaded));
	}

	TEST(StampingReturnsBeforeThePixelsArePainted) {
		Document document;
		document.setPaintWorkers(2);
		const Brush brush = roundBrush();
		document.setZoom(1.0f);
		document.whilePaintPaused([&]() {
			document.stampBrushDab({ 100, 100 }, brush);
			document.submitPaint();
			CHECK(document.activeLayer()->levels().isPreview(0, { 0, 0 }));
			CHECK(document.activeLayer()->tileIfExists({ 0, 0 }) == nullptr);
		});
		document.settlePaint();
		CHECK_EQUAL(0u, document.paintBacklog());
		CHECK_EQUAL(255, (int) pixelAt(document, 100, 100)[3]);
	}

	TEST(ZoomedInStrokeShowsOnThePreviewBeforeTheLayerIsPainted) {
		Document document;
		document.setZoom(1.0f);
		document.stampBrushDab({ 100, 100 }, roundBrush());
		const LayerLevels& levels = document.activeLayer()->levels();
		const Tile* preview = levels.basePreview({ 0, 0 });
		REQUIRE CHECK(preview != nullptr);
		CHECK_EQUAL(255, (int) preview->pixels()[((ualni) 100 * kTileSize + 100) * 4 + 3]);
		CHECK(preview->hasDirty());
		document.finishStroke();
		document.settlePaint();
		CHECK(levels.basePreview({ 0, 0 }) == nullptr);
		CHECK_EQUAL(255, (int) pixelAt(document, 100, 100)[3]);
	}

	TEST(PaintFinishedAfterACompositeStillReachesIt) {
		Document document;
		document.setPaintWorkers(2);
		Compositor compositor;
		compositor.update(document);

		document.whilePaintPaused([&]() {
			document.stampBrushDab({ 100, 100 }, roundBrush());
			document.submitPaint();
			compositor.update(document);
		});
		while (document.paintBacklog() > 0)
			std::this_thread::yield();
		document.whilePaintPaused([&]() { compositor.update(document); });

		const Tile* composite = compositor.tileAt({ 0, 0 });
		REQUIRE CHECK(composite != nullptr);
		CHECK_EQUAL(255, (int) composite->pixels()[((ualni) 100 * kTileSize + 100) * 4 + 3]);
	}

	TEST(HistoryAndUndoSeeTheFinishedStroke) {
		Document document;
		document.setPaintWorkers(3);
		scribble(document, roundBrush());
		document.finishStroke();
		const ualni painted = document.tileCount();
		CHECK_EQUAL(0u, document.paintBacklog());
		CHECK(painted > 0);

		document.undo();
		CHECK_EQUAL(0u, document.tileCount());
		document.redo();
		CHECK_EQUAL(painted, document.tileCount());
	}

	TEST(UndoDuringAStrokeKeepsWorkersOffRestoredTiles) {
		Document document;
		document.setPaintWorkers(2);
		scribble(document, roundBrush());
		document.finishStroke();
		Document expected;
		scribble(expected, roundBrush());
		expected.finishStroke();

		scribble(document, texturedBrush());
		document.finishStroke();
		scribble(document, texturedBrush());
		document.undo();
		CHECK_EQUAL(0u, document.paintBacklog());
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		CHECK(sameLayers(expected, document));
	}
}
