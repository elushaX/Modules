#include "Compositor.hpp"
#include "LayerLevels.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::sketch2d;

namespace {
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

	const uint1* pixel(const Tile* tile, int4 x, int4 y) {
		return tile->pixels() + ((ualni) y * kTileSize + (ualni) x) * 4;
	}
}

SUITE(MipLevels) {
	TEST(LevelIsFloorOfMinification) {
		CHECK_EQUAL(0, LayerLevels::levelForZoom(1.0f));
		CHECK_EQUAL(0, LayerLevels::levelForZoom(0.6f));
		CHECK_EQUAL(1, LayerLevels::levelForZoom(0.5f));
		CHECK_EQUAL(1, LayerLevels::levelForZoom(0.3f));
		CHECK_EQUAL(2, LayerLevels::levelForZoom(0.25f));
		CHECK_EQUAL(5, LayerLevels::levelForZoom(0.02f));
		CHECK_EQUAL(6, LayerLevels::levelForZoom(0.001f));
		CHECK_EQUAL(0, LayerLevels::levelForZoom(8.0f));
	}

	TEST(DownsampleAveragesTwoByTwoPremultiplied) {
		Tile source;
		uint1* pixels = source.pixels();
		const uint1 values[4][4] = { { 255, 0, 0, 255 }, { 0, 0, 0, 0 }, { 0, 0, 255, 255 }, { 0, 0, 0, 0 } };
		const int4 offsets[4] = { 0, 1, kTileSize, kTileSize + 1 };
		for (int4 sample = 0; sample < 4; sample++) {
			for (int4 channel = 0; channel < 4; channel++)
				pixels[offsets[sample] * 4 + channel] = values[sample][channel];
		}

		const Tile* quadrants[4] = { &source, nullptr, nullptr, nullptr };
		Tile target;
		LayerLevels::downsample(quadrants, target, 0, 0, kTileSize, kTileSize);

		const uint1* averaged = pixel(&target, 0, 0);
		CHECK_EQUAL(64, (int) averaged[0]);
		CHECK_EQUAL(0, (int) averaged[1]);
		CHECK_EQUAL(64, (int) averaged[2]);
		CHECK_EQUAL(128, (int) averaged[3]);
		CHECK_EQUAL(0, (int) pixel(&target, 200, 10)[3]);
	}

	TEST(FourTilesBecomeOneParentPerLevel) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 1, 0 }, 0, 255, 0, 255);
		fill(*document.activeLayer(), { 0, 1 }, 0, 0, 255, 255);
		fill(*document.activeLayer(), { 3, 3 }, 255, 255, 255, 255);

		LayerLevels mips;
		mips.update(*document.activeLayer(), 2);

		CHECK_EQUAL(2, mips.builtLevels());
		CHECK_EQUAL(2u, mips.tileCount(1));
		CHECK_EQUAL(1u, mips.tileCount(2));

		const Tile* parent = mips.tileAt(1, { 0, 0 });
		REQUIRE CHECK(parent != nullptr);
		CHECK_EQUAL(255, (int) pixel(parent, 10, 10)[0]);
		CHECK_EQUAL(255, (int) pixel(parent, kTileSize - 20, 10)[1]);
		CHECK_EQUAL(255, (int) pixel(parent, 10, kTileSize - 20)[2]);
		CHECK_EQUAL(0, (int) pixel(parent, kTileSize - 20, kTileSize - 20)[3]);

		const Tile* corner = mips.tileAt(1, { 1, 1 });
		REQUIRE CHECK(corner != nullptr);
		CHECK_EQUAL(255, (int) pixel(corner, kTileSize - 20, kTileSize - 20)[3]);
		CHECK_EQUAL(0, (int) pixel(corner, 10, 10)[3]);

		const Tile* top = mips.tileAt(2, { 0, 0 });
		REQUIRE CHECK(top != nullptr);
		CHECK_EQUAL(255, (int) pixel(top, 10, 10)[0]);
		CHECK_EQUAL(255, (int) pixel(top, kTileSize - 20, kTileSize - 20)[3]);
	}

	TEST(OnlyChangedParentsAreDownsampledAgain) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 4, 0 }, 0, 255, 0, 255);
		LayerLevels mips;
		mips.update(*document.activeLayer(), 1);
		CHECK_EQUAL(2u, mips.lastDownsampledTiles());

		mips.update(*document.activeLayer(), 1);
		CHECK_EQUAL(0u, mips.lastDownsampledTiles());

		uint1* pixels = document.activeLayer()->tileAt({ 4, 0 })->pixels();
		pixels[(20 * kTileSize + 20) * 4 + 0] = 255;
		document.activeLayer()->tileAt({ 4, 0 })->markDirty({ 20, 20, 1, 1 });
		mips.update(*document.activeLayer(), 1);
		CHECK_EQUAL(1u, mips.lastDownsampledTiles());
		CHECK_EQUAL(64, (int) pixel(mips.tileAt(1, { 2, 0 }), 10, 10)[0]);
	}

	TEST(ErasedRegionsDisappearFromMips) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		LayerLevels mips;
		mips.update(*document.activeLayer(), 3);
		CHECK_EQUAL(1u, mips.tileCount(3));

		document.clearActiveLayer();
		mips.update(*document.activeLayer(), 3);
		CHECK_EQUAL(0u, mips.tileCount(1));
		CHECK_EQUAL(0u, mips.tileCount(3));
	}

	TEST(BuiltLevelsStayCurrentWhileViewingFinerLevels) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		LayerLevels mips;
		mips.update(*document.activeLayer(), 2);

		fill(*document.activeLayer(), { 0, 0 }, 0, 255, 0, 255);
		mips.update(*document.activeLayer(), 0);
		CHECK_EQUAL(2, mips.builtLevels());
		CHECK_EQUAL(255, (int) pixel(mips.tileAt(2, { 0, 0 }), 10, 10)[1]);

		mips.update(*document.activeLayer(), 2);
		CHECK_EQUAL(0u, mips.lastDownsampledTiles());
	}

	TEST(IdleUpdatesTouchNoMipTiles) {
		Document document;
		for (int4 tile = 0; tile < 16; tile++)
			fill(*document.activeLayer(), { tile % 4, tile / 4 }, 255, 0, 0, 255);
		LayerLevels mips;
		mips.update(*document.activeLayer(), 2);
		CHECK_EQUAL(4u + 1u, mips.lastDownsampledTiles());

		mips.update(*document.activeLayer(), 2);
		CHECK_EQUAL(0u, mips.lastDownsampledTiles());

		document.activeLayer()->removeTile({ 3, 3 });
		document.activeLayer()->removeTile({ 2, 3 });
		document.activeLayer()->removeTile({ 3, 2 });
		document.activeLayer()->removeTile({ 2, 2 });
		mips.update(*document.activeLayer(), 2);
		CHECK(mips.tileAt(1, { 1, 1 }) == nullptr);
		CHECK_EQUAL(3u, mips.tileCount(1));
		CHECK_EQUAL(0, (int) pixel(mips.tileAt(2, { 0, 0 }), kTileSize - 20, kTileSize - 20)[3]);
	}

	TEST(UndoRefreshesMips) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		document.commit("Paint");
		LayerLevels mips;
		mips.update(*document.activeLayer(), 1);

		fill(*document.activeLayer(), { 0, 0 }, 0, 0, 255, 255);
		document.commit("Paint");
		mips.update(*document.activeLayer(), 1);
		CHECK_EQUAL(255, (int) pixel(mips.tileAt(1, { 0, 0 }), 10, 10)[2]);

		document.undo();
		mips.update(*document.activeLayer(), 1);
		CHECK_EQUAL(255, (int) pixel(mips.tileAt(1, { 0, 0 }), 10, 10)[0]);
		CHECK_EQUAL(0, (int) pixel(mips.tileAt(1, { 0, 0 }), 10, 10)[2]);
	}

	TEST(EachLayerKeepsItsOwnLevels) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		document.addLayer("Top");
		fill(*document.activeLayer(), { 2, 0 }, 0, 0, 255, 255);
		Compositor compositor;
		compositor.update(document, 1);
		const LayerLevels* bottom = compositor.levelsOf(*document.layers().front());
		const LayerLevels* top = compositor.levelsOf(*document.activeLayer());
		REQUIRE CHECK(bottom != nullptr && top != nullptr);
		CHECK_EQUAL(1u, bottom->tileCount(1));
		CHECK_EQUAL(1u, top->tileCount(1));
		CHECK(bottom->tileAt(1, { 0, 0 }) != nullptr);
		CHECK(bottom->tileAt(1, { 1, 0 }) == nullptr);
		CHECK(top->tileAt(1, { 1, 0 }) != nullptr);
	}

	TEST(CompositeAtLevelStacksTheLayerLevels) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 1, 0 }, 255, 0, 0, 255);
		document.addLayer("Top");
		document.activeLayer()->setOpacity(0.5f);
		fill(*document.activeLayer(), { 1, 0 }, 0, 0, 255, 255);
		Compositor compositor;
		compositor.update(document, 1);
		CHECK_EQUAL(1, compositor.level());
		CHECK_EQUAL(1u, compositor.tileCount());
		const Tile* composite = compositor.tileAt({ 0, 0 });
		REQUIRE CHECK(composite != nullptr);
		CHECK_EQUAL(255, (int) pixel(composite, 10, 10)[0]);
		CHECK_EQUAL(0, (int) pixel(composite, 10, 10)[2]);
		const uint1* mixed = pixel(composite, kTileSize - 10, 10);
		CHECK_CLOSE(128, (int) mixed[0], 2);
		CHECK_CLOSE(128, (int) mixed[2], 2);
	}

	TEST(ZoomedOutEditsNeverCompositeFullResolution) {
		Document document;
		for (int4 tile = 0; tile < 64; tile++)
			fill(*document.activeLayer(), { tile % 8, tile / 8 }, 255, 0, 0, 255);
		Compositor compositor;
		compositor.update(document, 3);
		CHECK_EQUAL(1u, compositor.tileCount());
		fill(*document.activeLayer(), { 5, 5 }, 0, 255, 0, 255);
		compositor.update(document, 3);
		CHECK_EQUAL(1u, compositor.lastCompositedTiles());
		CHECK_EQUAL((ualni) kTileSize * kTileSize, compositor.lastCompositedPixels());
		compositor.update(document, 3);
		CHECK_EQUAL(0u, compositor.lastCompositedTiles());
	}

	TEST(EachLevelKeepsItsOwnCompositeAndCatchesUp) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		fill(*document.activeLayer(), { 1, 1 }, 0, 255, 0, 255);
		Compositor compositor;
		compositor.update(document, 0);
		CHECK_EQUAL(2u, compositor.tileCount());
		compositor.update(document, 1);
		CHECK_EQUAL(1u, compositor.tileCount());
		CHECK_EQUAL(2u, compositor.tileCount(0));
		fill(*document.activeLayer(), { 1, 1 }, 0, 0, 255, 255);
		compositor.update(document, 1);
		CHECK_EQUAL(255, (int) pixel(compositor.tileAt({ 0, 0 }), kTileSize - 3, kTileSize - 3)[2]);
		compositor.update(document, 0);
		CHECK_EQUAL(2u, compositor.tileCount());
		CHECK_EQUAL(255, (int) pixel(compositor.tileAt({ 1, 1 }), 3, 3)[2]);
	}

	TEST(HiddenLayersAreLeftOutAtEveryLevel) {
		Document document;
		fill(*document.activeLayer(), { 0, 0 }, 255, 0, 0, 255);
		Compositor compositor;
		compositor.update(document, 2);
		CHECK_EQUAL(1u, compositor.tileCount());
		document.activeLayer()->setVisible(false);
		compositor.update(document, 2);
		CHECK_EQUAL(0u, compositor.tileCount());
	}

	TEST(BrushStrokeZoomedOutTouchesOnlyItsLevelTiles) {
		Document document;
		for (int4 tile = 0; tile < 256; tile++)
			fill(*document.activeLayer(), { tile % 16, tile / 16 }, 255, 255, 255, 255);
		Compositor compositor;
		compositor.update(document, 3);
		CHECK_EQUAL(4u, compositor.tileCount());
		Brush brush;
		brush.setSize(20);
		document.stampBrushDab({ 100, 100 }, brush);
		document.finishStroke();
		document.settlePaint();
		compositor.update(document, 3);
		CHECK_EQUAL(1u, compositor.lastCompositedTiles());
		const LayerLevels* levels = compositor.levelsOf(*document.activeLayer());
		REQUIRE CHECK(levels != nullptr);
		CHECK_EQUAL(1u, levels->lastDownsampledTiles() / 3);
	}

	TEST(OnlyTheChangedQuarterOfAParentIsDownsampled) {
		Document document;
		for (int4 tile = 0; tile < 4; tile++)
			fill(*document.activeLayer(), { tile % 2, tile / 2 }, 255, 0, 0, 255);
		LayerLevels levels;
		levels.update(*document.activeLayer(), 1);
		CHECK_EQUAL((ualni) kTileSize * kTileSize, levels.lastDownsampledPixels());
		document.activeLayer()->tileAt({ 1, 1 })->markDirty({ 0, 0, 1, 1 });
		levels.update(*document.activeLayer(), 1);
		CHECK_EQUAL((ualni) kTileSize * kTileSize / 4, levels.lastDownsampledPixels());
	}
}
