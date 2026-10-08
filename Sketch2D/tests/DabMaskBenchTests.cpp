#include "DabMask.hpp"
#include "StrokePainter.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>
#include <cstdio>

using namespace tp;
using namespace tp::sketch2d;

SUITE(BenchDabMasks) {
	TEST(CachedMasksAgainstShapeMathPerDabSize) {
		for (const halnf size : { 4.0f, 12.0f, 40.0f, 120.0f, 400.0f }) {
			Brush brush;
			brush.setSize(size);
			DabWork work;
			work.dab = PreparedDab(DabQuantizer::shape(brush.dabAt(1.0f)));
			work.strength = 0.5f;
			work.opacity = 1;
			Tile tile;
			const int4 dabs = (int4) (2.0e6f / (size * size)) + 200;
			double cachedMs = 0;
			double directMs = 0;
			for (int4 pass = 0; pass < 2; pass++) {
				const auto start = std::chrono::steady_clock::now();
				for (int4 index = 0; index < dabs; index++) {
					work.center = { 64.0f + (halnf) (index % 37) * 0.37f, 64.0f + (halnf) (index % 23) * 0.29f };
					if (pass == 0) StrokePainter::paint(work, tile, { 0, 0 }, 0, 0, kTileSize, kTileSize);
					else {
						const DabPlacement placement = DabPlacement::snap(work.center, size);
						const auto mask = DabMaskCache::rasterize(work.dab, placement, 1);
						CHECK(!mask->isEmpty());
					}
				}
				const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				(pass == 0 ? cachedMs : directMs) = ms;
			}
			std::printf(
				"[dab-mask] size %6.0f dabs %6d paint %8.2f ms  shape math only %8.2f ms\n", size, dabs, cachedMs, directMs
			);
		}
	}
}
