#include "Compositor.hpp"
#include "LayerLevels.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	constexpr halnf kTolerance = 0.002f;

	BlendPixel opaque(halnf r, halnf g, halnf b) { return { r, g, b, 1 }; }

	BlendPixel premultiplied(halnf r, halnf g, halnf b, halnf a) { return { r * a, g * a, b * a, a }; }

	void checkPixel(const BlendPixel& expected, const BlendPixel& actual) {
		CHECK_CLOSE(expected.r, actual.r, kTolerance);
		CHECK_CLOSE(expected.g, actual.g, kTolerance);
		CHECK_CLOSE(expected.b, actual.b, kTolerance);
		CHECK_CLOSE(expected.a, actual.a, kTolerance);
	}

	void fill(Layer& layer, const TileCoord& coord, uint1 r, uint1 g, uint1 b, uint1 a) {
		uint1* pixels = layer.tileAt(coord)->pixels();
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize; index++) {
			pixels[index * 4 + 0] = r;
			pixels[index * 4 + 1] = g;
			pixels[index * 4 + 2] = b;
			pixels[index * 4 + 3] = a;
		}
		layer.tileAt(coord)->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
	}

	const uint1* compositePixel(const Compositor& compositor, const TileCoord& coord) {
		const Tile* tile = compositor.tileAt(coord);
		return tile ? tile->pixels() : nullptr;
	}
}

SUITE(BlendFormulas) {
	TEST(NormalIsPremultipliedOver) {
		checkPixel(
			opaque(0.6f, 0.2f, 0.2f),
			blendPixel(BlendMode::Normal, opaque(0.2f, 0.2f, 0.2f), premultiplied(1, 0.2f, 0.2f, 0.5f), 1)
		);
		checkPixel(premultiplied(1, 0, 0, 0.25f), blendPixel(BlendMode::Normal, {}, premultiplied(1, 0, 0, 0.5f), 0.5f));
	}

	TEST(SeparableModesOnOpaquePixels) {
		const BlendPixel below = opaque(0.6f, 0.3f, 0.8f);
		const BlendPixel above = opaque(0.5f, 0.9f, 0.2f);

		checkPixel(opaque(0.30f, 0.27f, 0.16f), blendPixel(BlendMode::Multiply, below, above, 1));
		checkPixel(opaque(0.80f, 0.93f, 0.84f), blendPixel(BlendMode::Screen, below, above, 1));
		checkPixel(opaque(0.60f, 0.54f, 0.68f), blendPixel(BlendMode::Overlay, below, above, 1));
		checkPixel(opaque(0.60f, 0.90f, 0.80f), blendPixel(BlendMode::Lighten, below, above, 1));
		checkPixel(opaque(0.50f, 0.30f, 0.20f), blendPixel(BlendMode::Darken, below, above, 1));
		checkPixel(opaque(1.00f, 1.00f, 1.00f), blendPixel(BlendMode::ColorDodge, below, above, 1));
		checkPixel(opaque(0.20f, 0.2222f, 0.0f), blendPixel(BlendMode::ColorBurn, below, above, 1));
	}

	TEST(SoftDodgeBrightensByAboveAndClampsToBelowAlpha) {
		checkPixel(
			opaque(0.9f, 0.57f, 0.96f),
			blendPixel(BlendMode::SoftDodge, opaque(0.6f, 0.3f, 0.8f), opaque(0.5f, 0.9f, 0.2f), 1)
		);
	}

	TEST(SoftBurnDarkensByHalfOfInverseAbove) {
		checkPixel(
			opaque(0.45f, 0.285f, 0.48f),
			blendPixel(BlendMode::SoftBurn, opaque(0.6f, 0.3f, 0.8f), opaque(0.5f, 0.9f, 0.2f), 1)
		);
	}

	TEST(ColorModeKeepsBelowLuma) {
		const BlendPixel below = opaque(0.5f, 0.5f, 0.5f);
		const BlendPixel result = blendPixel(BlendMode::Color, below, opaque(1, 0, 0), 1);
		const halnf luma = 0.299f * result.r + 0.587f * result.g + 0.114f * result.b;
		CHECK_CLOSE(0.5f, luma, 0.01f);
		CHECK(result.r > result.g);
		CHECK_CLOSE(result.g, result.b, 0.01f);
	}

	TEST(ModesOverTransparentBelowActLikeNormal) {
		const BlendPixel above = premultiplied(0.4f, 0.7f, 0.1f, 0.8f);
		for (int4 mode = 1; mode < kBlendModeCount; mode++)
			checkPixel(above, blendPixel((BlendMode) mode, {}, above, 1));
	}

	TEST(OpacityScalesEveryMode) {
		const BlendPixel below = opaque(0.6f, 0.3f, 0.8f);
		checkPixel(below, blendPixel(BlendMode::Multiply, below, opaque(0.5f, 0.9f, 0.2f), 0));
		checkPixel(opaque(0.45f, 0.285f, 0.48f), blendPixel(BlendMode::Multiply, below, opaque(0.5f, 0.9f, 0.2f), 0.5f));
	}
}

SUITE(CompositorStack) {
	TEST(CompositesLayersBottomToTop) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 0, 0, 255, 255);
		document.addLayer("Top");
		fill(*document.activeLayer(), { 0, 0 }, 128, 0, 0, 128);

		Compositor compositor;
		compositor.update(document);
		const uint1* pixel = compositePixel(compositor, { 0, 0 });
		CHECK(pixel != nullptr);
		CHECK_EQUAL(128, (int) pixel[0]);
		CHECK_EQUAL(127, (int) pixel[2]);
		CHECK_EQUAL(255, (int) pixel[3]);
	}

	TEST(HiddenLayersAndOpacityAreRespected) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 255, 255, 255);
		document.setLayerOpacity(*document.activeLayer(), 0.5f);

		Compositor compositor;
		compositor.update(document);
		CHECK_EQUAL(128, (int) compositePixel(compositor, { 0, 0 })[3]);

		document.setLayerVisible(*document.activeLayer(), false);
		compositor.update(document);
		CHECK(compositePixel(compositor, { 0, 0 }) == nullptr);
	}

	TEST(ClippedLayerOnlyPaintsInsideBase) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 0, 0, 0, 0);
		uint1* base = document.activeLayer()->tileAt({ 0, 0 })->pixels();
		base[0] = base[1] = base[2] = base[3] = 255;

		document.addLayer("Clip");
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		document.activeLayer()->setClipped(true);

		Compositor compositor;
		compositor.update(document);
		const uint1* pixels = compositePixel(compositor, { 0, 0 });
		CHECK_EQUAL(255, (int) pixels[0]);
		CHECK_EQUAL(0, (int) pixels[1]);
		CHECK_EQUAL(0, (int) pixels[4 + 3]);
	}

	TEST(ClippedLayerHidesWithItsBase) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 255, 255, 255);
		Layer* base = document.activeLayer();
		document.addLayer("Clip");
		fill(*document.activeLayer(), { 1, 0 }, 255, 0, 0, 255);
		document.activeLayer()->setClipped(true);
		base->setVisible(false);

		Compositor compositor;
		compositor.update(document);
		CHECK(compositePixel(compositor, { 0, 0 }) == nullptr);
		CHECK(compositePixel(compositor, { 1, 0 }) == nullptr);
	}

	TEST(UnchangedTilesAreNotRecomposited) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 1, 0 }, 0, 255, 0, 255);

		Compositor compositor;
		compositor.update(document);
		CHECK_EQUAL(2u, compositor.lastCompositedTiles());

		compositor.update(document);
		CHECK_EQUAL(0u, compositor.lastCompositedTiles());

		document.activeLayer()->tileAt({ 1, 0 })->markDirty({ 0, 0, 4, 4 });
		compositor.update(document);
		CHECK_EQUAL(1u, compositor.lastCompositedTiles());

		document.activeLayer()->setBlend(BlendMode::Multiply);
		compositor.update(document);
		CHECK_EQUAL(2u, compositor.lastCompositedTiles());
	}

	TEST(UndoRecompositesRestoredTiles) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		document.commit("Paint");
		Compositor compositor;
		compositor.update(document);

		fill(*document.activeLayer(), { 0, 0 }, 0, 255, 0, 255);
		document.commit("Paint");
		compositor.update(document);
		CHECK_EQUAL(255, (int) compositePixel(compositor, { 0, 0 })[1]);

		document.undo();
		compositor.update(document);
		CHECK_EQUAL(255, (int) compositePixel(compositor, { 0, 0 })[0]);
		CHECK_EQUAL(0, (int) compositePixel(compositor, { 0, 0 })[1]);
	}

	TEST(CompositeTilesDisappearWithTheirSources) {
		Document document;
		fill(*document.activeLayer(), { 3, 3 }, 255, 0, 0, 255);
		Compositor compositor;
		compositor.update(document);
		CHECK_EQUAL(1u, compositor.tileCount());

		document.clearActiveLayer();
		compositor.update(document);
		CHECK_EQUAL(0u, compositor.tileCount());
	}

	TEST(DistantEditsInOneTileRecompositeOnlyTheirBands) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		Compositor compositor;
		compositor.update(document);

		Tile* tile = document.activeLayer()->tileAt({ 0, 0 });
		tile->markDirty({ 0, 0, 4, 4 });
		tile->markDirty({ (halnf) kTileSize - 6, (halnf) kTileSize - 6, 4, 4 });
		compositor.update(document);
		CHECK_EQUAL(32u, compositor.lastCompositedPixels());
	}

	TEST(IntegerNormalStackMatchesTheFloatBlend) {
		Document document;
		uint1* below = document.activeLayer()->tileAt({ 0, 0 })->pixels();
		document.activeLayer()->setOpacity(0.7f);
		document.addLayer("Top");
		uint1* above = document.activeLayer()->tileAt({ 0, 0 })->pixels();
		document.activeLayer()->setOpacity(0.45f);
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize; index++) {
			const uint1 alphaBelow = (uint1) (index * 7 % 256);
			const uint1 alphaAbove = (uint1) (index * 13 % 256);
			below[index * 4 + 3] = alphaBelow;
			above[index * 4 + 3] = alphaAbove;
			for (int4 channel = 0; channel < 3; channel++) {
				below[index * 4 + channel] = (uint1) ((index * (channel + 3)) % (alphaBelow + 1));
				above[index * 4 + channel] = (uint1) ((index * (channel + 5)) % (alphaAbove + 1));
			}
		}

		Compositor compositor;
		compositor.update(document);
		const uint1* result = compositePixel(compositor, { 0, 0 });

		int4 worst = 0;
		const Layer& bottom = *document.layers()[0];
		const Layer& top = *document.layers()[1];
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize; index++) {
			BlendPixel expected;
			for (const Layer* layer : { &bottom, &top }) {
				const uint1* px = layer->tileIfExists({ 0, 0 })->pixels() + index * 4;
				const BlendPixel pixel{ px[0] / 255.0f, px[1] / 255.0f, px[2] / 255.0f, px[3] / 255.0f };
				if (pixel.a > 0) expected = blendPixel(BlendMode::Normal, expected, pixel, layer->opacity());
			}
			const halnf channels[4] = { expected.r, expected.g, expected.b, expected.a };
			for (int4 channel = 0; channel < 4; channel++)
				worst = std::max(
					worst, std::abs((int4) result[index * 4 + channel] - (int4) std::lround(channels[channel] * 255.0f))
				);
		}
		CHECK(worst <= 2);
	}

	TEST(ParallelCompositingAndMipsMatchSerial) {
		Document document;
		for (int4 tile = 0; tile < 12; tile++)
			fill(*document.activeLayer(), { tile % 4, tile / 4 }, (uint1) (tile * 20), 40, 200, 255);
		document.addLayer("Top");
		document.activeLayer()->setBlend(BlendMode::Multiply);
		for (int4 tile = 0; tile < 12; tile += 2)
			fill(*document.activeLayer(), { tile % 4, tile / 4 }, 255, (uint1) (tile * 15), 0, 180);

		ParallelFor parallel(4);
		Compositor serial;
		Compositor threaded;
		threaded.setParallel(&parallel);
		serial.update(document);
		Compositor serialLevel;
		Compositor threadedLevel;
		threadedLevel.setParallel(&parallel);
		serialLevel.update(document, 2);
		for (const auto& layer : document.layers())
			layer->forEachTile([](const TileCoord&, Tile* tile) {
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
			});
		threaded.update(document);
		threadedLevel.update(document, 2);

		CHECK_EQUAL(serial.tileCount(), threaded.tileCount());
		bool same = true;
		serial.forEachTile([&](const TileCoord& coord, Tile* tile) {
			const Tile* other = threaded.tileAt(coord);
			same = same && other && std::memcmp(tile->pixels(), other->pixels(), (size_t) kTileSize * kTileSize * 4) == 0;
		});
		CHECK(serialLevel.tileCount() > 0);
		serialLevel.forEachTile([&](const TileCoord& coord, Tile* tile) {
			const Tile* other = threadedLevel.tileAt(coord);
			same = same && other && std::memcmp(tile->pixels(), other->pixels(), (size_t) kTileSize * kTileSize * 4) == 0;
		});
		CHECK(same);
	}

	TEST(IdleFramesVisitOnlyChangedTiles) {
		Document document;
		for (int4 tile = 0; tile < 64; tile++)
			fill(*document.activeLayer(), { tile % 8, tile / 8 }, 10, 20, 30, 255);
		Compositor compositor;
		compositor.update(document);
		CHECK_EQUAL(64u, compositor.lastChanged().size());

		compositor.update(document);
		CHECK_EQUAL(0u, compositor.lastChanged().size());
		CHECK_EQUAL(0u, compositor.lastCompositedTiles());

		fill(*document.activeLayer(), { 3, 3 }, 200, 0, 0, 255);
		compositor.update(document);
		REQUIRE CHECK_EQUAL(1u, compositor.lastChanged().size());
		CHECK(compositor.lastChanged()[0] == TileCoord({ 3, 3 }));
		CHECK_EQUAL(200, (int) compositePixel(compositor, { 3, 3 })[0]);
	}

	TEST(RemovedAndRestoredTilesReachTheComposite) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 1, 0 }, 0, 255, 0, 255);
		document.commit("Paint");
		Compositor compositor;
		compositor.update(document);

		document.activeLayer()->removeTile({ 1, 0 });
		compositor.update(document);
		CHECK(compositor.tileAt({ 1, 0 }) == nullptr);
		CHECK_EQUAL(1u, compositor.lastRemoved().size());

		fill(*document.activeLayer(), { 0, 0 }, 0, 0, 255, 255);
		document.commit("Paint");
		compositor.update(document);
		document.undo();
		compositor.update(document);
		CHECK_EQUAL(255, (int) compositePixel(compositor, { 0, 0 })[0]);
		CHECK_EQUAL(255, (int) compositePixel(compositor, { 1, 0 })[1]);
	}

	TEST(LayerPropertyChangesRecompositeEverything) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 5, 5 }, 255, 0, 0, 255);
		Compositor compositor;
		compositor.update(document);

		document.setLayerOpacity(*document.activeLayer(), 0.5f);
		compositor.update(document);
		CHECK_EQUAL(2u, compositor.lastCompositedTiles());
		CHECK_EQUAL(128, (int) compositePixel(compositor, { 5, 5 })[3]);

		document.setLayerVisible(*document.activeLayer(), false);
		compositor.update(document);
		CHECK_EQUAL(0u, compositor.tileCount());
	}

	TEST(SingleOpaqueLayerIsCopiedExactly) {
		Document document;
		uint1* pixels = document.activeLayer()->tileAt({ 0, 0 })->pixels();
		for (ualni index = 0; index < (ualni) kTileSize * kTileSize * 4; index++)
			pixels[index] = (uint1) (index * 31 % 251);
		Compositor compositor;
		compositor.update(document);
		CHECK(std::memcmp(pixels, compositePixel(compositor, { 0, 0 }), (size_t) kTileSize * kTileSize * 4) == 0);
	}

	TEST(SmallEditsRecompositeOnlyTheChangedRect) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		Compositor compositor;
		compositor.update(document);
		Tile* composite = compositor.tileAt({ 0, 0 });
		composite->clearDirty();

		uint1* pixels = document.activeLayer()->tileAt({ 0, 0 })->pixels();
		pixels[(20 * kTileSize + 20) * 4 + 1] = 255;
		document.activeLayer()->tileAt({ 0, 0 })->markDirty({ 20, 20, 1, 1 });
		compositor.update(document);

		CHECK(composite->hasDirty());
		CHECK_CLOSE(1, composite->dirtyRect().size.x, 0.01f);
		CHECK_CLOSE(1, composite->dirtyRect().size.y, 0.01f);
		CHECK_EQUAL(255, (int) composite->pixels()[(20 * kTileSize + 20) * 4 + 1]);
	}
}
