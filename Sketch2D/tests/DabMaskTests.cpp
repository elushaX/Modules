#include "DabMask.hpp"
#include "Document.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <tuple>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	DabShape roundDab(halnf size, halnf hardness = 0.5f) {
		DabShape dab;
		dab.size = size;
		dab.opacity = 1;
		dab.flow = 1;
		dab.hardness = hardness;
		return dab;
	}
}

SUITE(DabMasks) {
	TEST(PositionsSnapCoarserForBiggerDabsLikeLeonardo) {
		CHECK_EQUAL(16, DabPlacement::stepsFor(8));
		CHECK_EQUAL(16, DabPlacement::stepsFor(16));
		CHECK_EQUAL(8, DabPlacement::stepsFor(17));
		CHECK_EQUAL(4, DabPlacement::stepsFor(65));
		CHECK_EQUAL(2, DabPlacement::stepsFor(257));

		const DabPlacement fine = DabPlacement::snap({ 10.03f, 20.97f }, 8);
		CHECK_CLOSE(10.0f, fine.center.x, 1e-5f);
		CHECK_CLOSE(21.0f, fine.center.y, 1e-5f);
		const DabPlacement coarse = DabPlacement::snap({ 10.3f, -4.6f }, 300);
		CHECK_CLOSE(10.5f, coarse.center.x, 1e-5f);
		CHECK_CLOSE(-4.5f, coarse.center.y, 1e-5f);
		CHECK_EQUAL(1, coarse.phaseX);
		CHECK_EQUAL(1, coarse.phaseY);
	}

	TEST(DabsWithTheSameShapeAndPhaseShareOneMask) {
		DabMaskCache& cache = DabMaskCache::shared();
		cache.clear();
		const PreparedDab dab(roundDab(12));
		const auto first = cache.mask(dab, DabPlacement::snap({ 3.25f, 7.5f }, 12), 1);
		const auto moved = cache.mask(dab, DabPlacement::snap({ 103.25f, -40.5f }, 12), 1);
		const auto shifted = cache.mask(dab, DabPlacement::snap({ 3.5f, 7.5f }, 12), 1);
		REQUIRE CHECK(first != nullptr);
		CHECK(first == moved);
		CHECK(first != shifted);
		CHECK_EQUAL(2u, cache.size());
		CHECK_EQUAL(1u, cache.hits());
	}

	TEST(CachedMaskMatchesDirectCoverage) {
		const PreparedDab dab(roundDab(20, 0.3f));
		const DabPlacement placement = DabPlacement::snap({ 5.4375f, 9.875f }, 20);
		const auto mask = DabMaskCache::rasterize(dab, placement, 1);
		const halnf fx = (halnf) placement.phaseX / (halnf) placement.steps;
		const halnf fy = (halnf) placement.phaseY / (halnf) placement.steps;
		halnf worst = 0;
		for (int4 y = mask->top; y < mask->top + mask->height; y++)
			for (int4 x = mask->left; x < mask->left + mask->width; x++) {
				halnf distance = 0;
				const halnf expected = dab.coverage((halnf) x + 0.5f - fx, (halnf) y + 0.5f - fy, distance);
				const halnf cached = (halnf) mask->coverage[mask->index(x, y)] / (halnf) DabMask::kOne;
				worst = std::max(worst, std::abs(expected - cached));
			}
		CHECK(worst < 1.0f / 4096.0f);
	}

	TEST(HugeDabsAreNotCached) {
		const PreparedDab dab(roundDab(2000));
		CHECK(DabMaskCache::shared().mask(dab, DabPlacement::snap({ 0, 0 }, 2000), 1) == nullptr);
		CHECK(DabMaskCache::shared().mask(dab, DabPlacement::snap({ 0, 0 }, 2000 / 16.0f), 16) != nullptr);
	}

	TEST(StrokeOfRepeatedDabsReusesMasks) {
		DabMaskCache::shared().clear();
		Document document;
		Brush brush;
		brush.setSize(10);
		for (int4 dab = 0; dab < 200; dab++)
			document.stampBrushDab({ 20.0f + (halnf) dab * 0.5f, 40.0f }, brush, 1.0f);
		document.settlePaint();
		CHECK(DabMaskCache::shared().misses() <= 16u);
		CHECK(DabMaskCache::shared().hits() > 150u);
	}

	TEST(PressureStrokeRasterizesEachShapeAndPhaseOnce) {
		DabMaskCache::shared().clear();
		Document document;
		Brush brush;
		brush.apply(*findBrushPreset(BrushType::Ink));
		brush.setSize(30);
		std::set<std::tuple<halnf, halnf, halnf, halnf, halnf, int4, int4>> distinct;
		for (int4 dab = 0; dab < 400; dab++) {
			const halnf pressure = 0.3f + 0.6f * (halnf) (dab % 100) / 100.0f;
			const Vec2F center = { 20.0f + (halnf) dab * 0.7f, 40.0f };
			document.stampBrushDab(center, brush, pressure);
			const DabShape shape = DabQuantizer::shape(brush.dabAt(pressure));
			const DabPlacement placement = DabPlacement::snap(center, shape.size);
			distinct.insert(
				{ shape.size,
					shape.interior,
					shape.hardness,
					shape.roundness,
					shape.angleDegrees,
					placement.phaseX,
					placement.phaseY }
			);
		}
		document.settlePaint();
		CHECK_EQUAL(distinct.size(), (size_t) DabMaskCache::shared().misses());
		CHECK(distinct.size() < 200u);
	}
}
