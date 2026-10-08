#include "Stabilizer.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>
#include <random>
#include <vector>

using namespace tp;
using namespace tp::sketch2d;

namespace {
	std::vector<StrokePoint> feed(Stabilizer& stabilizer, const std::vector<Vec2F>& points, halnf amount) {
		std::vector<StrokePoint> output;
		stabilizer.begin({ points.front(), 1 }, amount);
		for (size_t index = 1; index < points.size(); index++) {
			stabilizer.moveTo({ points[index], 1 });
			for (const StrokePoint& point : stabilizer.takeOutput())
				output.push_back(point);
		}
		stabilizer.finish();
		for (const StrokePoint& point : stabilizer.takeOutput())
			output.push_back(point);
		return output;
	}

	std::vector<Vec2F> jitteryLine(int count, halnf jitter, uint32_t seed) {
		std::mt19937 random(seed);
		std::uniform_real_distribution<halnf> noise(-jitter, jitter);
		std::vector<Vec2F> points;
		for (int index = 0; index < count; index++)
			points.push_back({ (halnf) index * 3.0f, noise(random) });
		return points;
	}

	halnf deviationFromAxis(const std::vector<StrokePoint>& points, halnf fromX, halnf toX) {
		halnf sum = 0;
		int count = 0;
		for (const StrokePoint& point : points) {
			if (point.position.x < fromX || point.position.x > toX) continue;
			sum += point.position.y * point.position.y;
			count++;
		}
		return count ? std::sqrt(sum / (halnf) count) : 0;
	}

	halnf longestGap(const std::vector<StrokePoint>& points) {
		halnf longest = 0;
		for (size_t index = 1; index < points.size(); index++)
			longest = std::max(longest, (halnf) (points[index].position - points[index - 1].position).length());
		return longest;
	}
}

SUITE(Stabilizer) {
	TEST(ZeroAmountPassesPointsThrough) {
		Stabilizer stabilizer;
		const std::vector<StrokePoint> output = feed(stabilizer, { { 0, 0 }, { 10, 0 }, { 10, 10 } }, 0);
		REQUIRE CHECK_EQUAL(3u, output.size());
		CHECK_CLOSE(10, output[1].position.x, 1e-5f);
		CHECK_CLOSE(10, output[2].position.y, 1e-5f);
	}

	TEST(StrokeStartsExactlyUnderThePen) {
		Stabilizer stabilizer;
		stabilizer.begin({ { 42, 17 }, 0.3f }, 0.8f);
		const std::vector<StrokePoint> output = stabilizer.takeOutput();
		REQUIRE CHECK_EQUAL(1u, output.size());
		CHECK_CLOSE(42, output[0].position.x, 1e-5f);
		CHECK_CLOSE(0.3f, output[0].pressure, 1e-5f);
	}

	TEST(HandJitterIsSmoothedOut) {
		const std::vector<Vec2F> line = jitteryLine(200, 4, 11);
		Stabilizer raw;
		Stabilizer smooth;
		const halnf rawDeviation = deviationFromAxis(feed(raw, line, 0), 100, 500);
		const halnf smoothDeviation = deviationFromAxis(feed(smooth, line, Stabilizer::kDefaultAmount), 100, 500);
		CHECK(rawDeviation > 2.0f);
		CHECK(smoothDeviation < rawDeviation * 0.45f);
	}

	TEST(MoreStabilizationSmoothsMore) {
		const std::vector<Vec2F> line = jitteryLine(300, 4, 5);
		Stabilizer light;
		Stabilizer strong;
		CHECK(deviationFromAxis(feed(strong, line, 1), 200, 800) < deviationFromAxis(feed(light, line, 0.15f), 200, 800));
	}

	TEST(SmoothingDoesNotDependOnEventRate) {
		std::vector<Vec2F> sparse;
		std::vector<Vec2F> dense;
		for (int index = 0; index <= 20; index++)
			sparse.push_back({ (halnf) index * 10.0f, index % 2 ? 6.0f : -6.0f });
		for (int index = 0; index <= 200; index++) {
			const int segment = index / 10;
			const halnf amount = (halnf) (index % 10) / 10.0f;
			const Vec2F from = sparse[(size_t) segment];
			const Vec2F to = sparse[(size_t) std::min(segment + 1, 20)];
			dense.push_back(from + (to - from) * amount);
		}
		Stabilizer a;
		Stabilizer b;
		const std::vector<StrokePoint> fromSparse = feed(a, sparse, 0.5f);
		const std::vector<StrokePoint> fromDense = feed(b, dense, 0.5f);
		CHECK_CLOSE(fromSparse.back().position.x, fromDense.back().position.x, 0.5f);
		CHECK_CLOSE(deviationFromAxis(fromSparse, 40, 160), deviationFromAxis(fromDense, 40, 160), 0.5f);
	}

	TEST(OutputIsDenseEvenForFastStrokes) {
		Stabilizer stabilizer;
		const std::vector<StrokePoint> output = feed(stabilizer, { { 0, 0 }, { 300, 0 }, { 300, 300 } }, 0.5f);
		CHECK(longestGap(output) < 2.0f);
	}

	TEST(RopeIgnoresTinyWobblesAroundAPoint) {
		Stabilizer stabilizer;
		stabilizer.begin({ { 0, 0 }, 1 }, 1);
		for (int index = 0; index < 50; index++)
			stabilizer.moveTo({ { index % 2 ? 2.0f : -2.0f, index % 3 ? 1.5f : -1.5f }, 1 });
		const std::vector<StrokePoint> output = stabilizer.takeOutput();
		halnf farthest = 0;
		for (const StrokePoint& point : output)
			farthest = std::max(farthest, (halnf) point.position.length());
		CHECK(farthest < 1.0f);
	}

	TEST(RestingPenLetsTheLineCatchUp) {
		Stabilizer stabilizer;
		stabilizer.begin({ { 0, 0 }, 1 }, Stabilizer::kDefaultAmount);
		stabilizer.moveTo({ { 100, 0 }, 1 });
		CHECK(!stabilizer.isSettled());
		const halnf lagging = stabilizer.smoothed().position.x;
		CHECK(lagging < 95);

		stabilizer.advance(16);
		CHECK_CLOSE(lagging, stabilizer.smoothed().position.x, 1e-4f);

		for (int frame = 0; frame < 120; frame++)
			stabilizer.advance(16);
		CHECK(stabilizer.isSettled());
		CHECK_CLOSE(100, stabilizer.takeOutput().back().position.x, 0.1f);
	}

	TEST(MovingResetsTheIdleTimer) {
		Stabilizer stabilizer;
		stabilizer.begin({ { 0, 0 }, 1 }, 0.5f);
		stabilizer.moveTo({ { 50, 0 }, 1 });
		stabilizer.advance(30);
		stabilizer.moveTo({ { 51, 0 }, 1 });
		const halnf before = stabilizer.smoothed().position.x;
		stabilizer.advance(30);
		CHECK_CLOSE(before, stabilizer.smoothed().position.x, 1e-4f);
	}

	TEST(FinishEndsWhereTheSmoothedLineIsNotWhereThePenJumped) {
		Stabilizer stabilizer;
		const std::vector<StrokePoint> output = feed(stabilizer, { { 0, 0 }, { 200, 0 } }, 0.5f);
		CHECK(output.back().position.x < 200);
		CHECK(!stabilizer.isActive());
	}

	TEST(PressureIsSmoothedWithPosition) {
		Stabilizer stabilizer;
		stabilizer.begin({ { 0, 0 }, 0 }, 0.5f);
		stabilizer.moveTo({ { 100, 0 }, 1 });
		const std::vector<StrokePoint> output = stabilizer.takeOutput();
		for (size_t index = 1; index < output.size(); index++)
			CHECK(output[index].pressure >= output[index - 1].pressure - 1e-5f);
		CHECK(output.back().pressure > 0.5f);
		CHECK(output.back().pressure < 1.0f);
	}
}
