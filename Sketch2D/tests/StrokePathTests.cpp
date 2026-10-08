#include "StrokePath.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	std::vector<StrokePath::Sample> walk(StrokePath& path, StrokePath::Walk& cursor, halnf spacing, bool finished) {
		std::vector<StrokePath::Sample> dabs;
		path.advance(
			cursor,
			[spacing](halnf) { return spacing; },
			[&dabs](const StrokePath::Sample& dab) { dabs.push_back(dab); },
			finished
		);
		return dabs;
	}

	std::vector<StrokePath::Sample> walk(StrokePath& path, halnf spacing, bool finished) {
		StrokePath::Walk cursor;
		return walk(path, cursor, spacing, finished);
	}

	halnf gap(const StrokePath::Sample& a, const StrokePath::Sample& b) {
		const Vec2F d = b.position - a.position;
		return std::sqrt(d.x * d.x + d.y * d.y);
	}
}

SUITE(StrokePath) {
	TEST(DabsSitAtExactSpacingWhateverTheInputRate) {
		for (int samples : { 2, 5, 50, 400 }) {
			StrokePath path;
			path.begin({ { 0, 0 }, 1 });
			for (int index = 1; index <= samples; index++)
				path.add({ { 1000.0f * (halnf) index / (halnf) samples, 0.0f }, 1 });
			const std::vector<StrokePath::Sample> dabs = walk(path, 10, true);
			REQUIRE CHECK_EQUAL(100u, dabs.size());
			for (size_t index = 0; index < dabs.size(); index++)
				CHECK_CLOSE(10.0f * (halnf) (index + 1), dabs[index].position.x, 0.01f);
		}
	}

	TEST(TheWalkResumesWhenMoreSamplesArrive) {
		StrokePath once;
		StrokePath streamed;
		StrokePath::Walk cursor;
		once.begin({ { 0, 0 }, 1 });
		streamed.begin({ { 0, 0 }, 1 });
		std::vector<StrokePath::Sample> pieces;
		for (int index = 1; index <= 40; index++) {
			const StrokePath::Sample sample = { { (halnf) index * 13.0f, std::sin((halnf) index * 0.3f) * 40.0f }, 1 };
			once.add(sample);
			streamed.add(sample);
			for (const auto& dab : walk(streamed, cursor, 7, false))
				pieces.push_back(dab);
		}
		for (const auto& dab : walk(streamed, cursor, 7, true))
			pieces.push_back(dab);
		const std::vector<StrokePath::Sample> whole = walk(once, 7, true);
		REQUIRE CHECK_EQUAL(whole.size(), pieces.size());
		for (size_t index = 0; index < whole.size(); index++) {
			CHECK_CLOSE(whole[index].position.x, pieces[index].position.x, 0.01f);
			CHECK_CLOSE(whole[index].position.y, pieces[index].position.y, 0.01f);
		}
	}

	TEST(TheCurveIsSmoothThroughSparseSamples) {
		StrokePath path;
		path.begin({ { 0, 0 }, 1 });
		path.add({ { 100, 100 }, 1 });
		path.add({ { 200, 0 }, 1 });
		path.add({ { 300, 100 }, 1 });
		const std::vector<StrokePath::Sample> dabs = walk(path, 2, true);
		REQUIRE CHECK(dabs.size() > 100);
		for (size_t index = 1; index < dabs.size(); index++)
			CHECK_CLOSE(2.0f, gap(dabs[index - 1], dabs[index]), 0.01f);
		const StrokePath::Sample middle = path.at(0.5f);
		CHECK(std::abs(middle.position.x - middle.position.y) > 1.0f);
		CHECK_CLOSE(100.0f, path.at(1.0f).position.x, 0.001f);
		CHECK_CLOSE(100.0f, path.at(1.0f).position.y, 0.001f);
	}

	TEST(PressureFollowsTheSamples) {
		StrokePath path;
		path.begin({ { 0, 0 }, 0.0f });
		path.add({ { 100, 0 }, 1.0f });
		const std::vector<StrokePath::Sample> dabs = walk(path, 10, true);
		REQUIRE CHECK_EQUAL(10u, dabs.size());
		for (size_t index = 1; index < dabs.size(); index++)
			CHECK(dabs[index].pressure > dabs[index - 1].pressure);
		CHECK_CLOSE(1.0f, dabs.back().pressure, 0.02f);
	}

	TEST(UnfinishedWalkStopsBeforeTheLastSegment) {
		StrokePath path;
		path.begin({ { 0, 0 }, 1 });
		path.add({ { 100, 0 }, 1 });
		path.add({ { 200, 0 }, 1 });
		StrokePath::Walk cursor;
		const std::vector<StrokePath::Sample> early = walk(path, cursor, 10, false);
		CHECK_EQUAL(10u, early.size());
		CHECK(cursor.parameter <= 1.0f);
		const std::vector<StrokePath::Sample> rest = walk(path, cursor, 10, true);
		CHECK_EQUAL(10u, rest.size());
	}

	TEST(SpacingFollowsPressureDependentSize) {
		StrokePath path;
		path.begin({ { 0, 0 }, 0.2f });
		path.add({ { 1000, 0 }, 1.0f });
		std::vector<StrokePath::Sample> dabs;
		StrokePath::Walk cursor;
		path.advance(
			cursor,
			[](halnf pressure) { return 20.0f * pressure; },
			[&dabs](const StrokePath::Sample& dab) { dabs.push_back(dab); },
			true
		);
		REQUIRE CHECK(dabs.size() > 10);
		CHECK(gap(dabs[0], dabs[1]) < gap(dabs[dabs.size() - 2], dabs.back()));
	}

	TEST(StandingStillAddsNoSamples) {
		StrokePath path;
		path.begin({ { 5, 5 }, 0.5f });
		path.add({ { 5, 5 }, 0.8f });
		CHECK_EQUAL(1u, path.sampleCount());
		CHECK_CLOSE(0.8f, path.at(0).pressure, 0.001f);
	}

	TEST(FinishedWalkEmitsNoMoreDabsThanTheLengthAllows) {
		StrokePath path;
		path.begin({ { 0, 0 }, 1 });
		const Vec2F points[] = { { 10.0f, 0.0f },     { 10.0001f, 0.0f }, { 20.0f, 5.0f },
														 { 20.0f, 5.00001f }, { 0.0f, 0.0f },     { 0.5f, 0.2f } };
		for (const Vec2F& point : points)
			path.add({ point, 1 });
		halnf length = 0;
		for (halnf parameter = 0; parameter < (halnf) path.sampleCount() - 1.0f; parameter += 0.001f)
			length += (path.at(parameter + 0.001f).position - path.at(parameter).position).length();
		const std::vector<StrokePath::Sample> dabs = walk(path, 0.5f, true);
		CHECK(dabs.size() <= (size_t) (length / 0.5f) + 2);
		CHECK(dabs.size() + 2 >= (size_t) (length / 0.5f));
	}
}

SUITE(BenchStrokePath) {
	TEST(WalkCostPerDab) {
		StrokePath path;
		path.begin({ { 0, 0 }, 1 });
		for (int index = 1; index <= 250; index++)
			path.add({ { (halnf) index * 240.0f, std::sin((halnf) index * 0.2f) * 300.0f }, 1 });
		size_t dabs = 0;
		StrokePath::Walk cursor;
		const auto start = std::chrono::steady_clock::now();
		path.advance(cursor, [](halnf) { return 0.5f; }, [&dabs](const StrokePath::Sample&) { dabs++; }, true);
		const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		std::printf("[walk] dabs %zu %.2f ms %.3f us per dab\n", dabs, ms, ms * 1000.0 / (double) dabs);
	}
}
