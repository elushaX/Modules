#include "UnitTest++/UnitTest++.h"

#include "InputCoalescer.hpp"
#include "TouchGesture.hpp"
#include "WindowInput.hpp"

#include <cmath>
#include <numbers>

using namespace tp;

namespace {
	std::vector<TouchGesture> feed(TouchGestureTracker& tracker, std::vector<TouchPoint> touches, uint32_t timeMs = 0) {
		std::vector<TouchGesture> gestures;
		tracker.update(touches, gestures, timeMs);
		return gestures;
	}

	int32_t tapFingersOf(const std::vector<TouchGesture>& gestures) {
		return gestures.empty() || gestures.back().phase != GesturePhase::End ? -1 : gestures.back().tapFingers;
	}

	class GestureLog : public WindowInputObserver {
	public:
		void onGesture(const TouchGesture& gesture) override { gestures.push_back(gesture); }
		std::vector<TouchGesture> gestures;
	};
}

SUITE(TouchGestureTracker) {
	TEST(OneFingerProducesNoGesture) {
		TouchGestureTracker tracker;

		CHECK(feed(tracker, { { 1, { 10, 10 } } }).empty());
		CHECK(!tracker.isActive());
	}

	TEST(SecondFingerBeginsAtTheMidpoint) {
		TouchGestureTracker tracker;

		const auto gestures = feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } });

		REQUIRE CHECK_EQUAL(1u, gestures.size());
		CHECK(gestures[0].phase == GesturePhase::Begin);
		CHECK_CLOSE(50.0f, gestures[0].center.x, 0.001f);
		CHECK(tracker.isActive());
	}

	TEST(MovingBothFingersPans) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } });

		const auto gestures = feed(tracker, { { 1, { 10, 20 } }, { 2, { 110, 20 } } });

		REQUIRE CHECK_EQUAL(1u, gestures.size());
		CHECK(gestures[0].phase == GesturePhase::Update);
		CHECK_CLOSE(10.0f, gestures[0].pan.x, 0.001f);
		CHECK_CLOSE(20.0f, gestures[0].pan.y, 0.001f);
		CHECK_CLOSE(1.0f, gestures[0].scale, 0.001f);
		CHECK_CLOSE(0.0f, gestures[0].rotation, 0.001f);
	}

	TEST(SpreadingFingersScalesAroundTheMidpoint) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 50, 0 } }, { 2, { 150, 0 } } });

		const auto gestures = feed(tracker, { { 1, { 0, 0 } }, { 2, { 200, 0 } } });

		REQUIRE CHECK_EQUAL(1u, gestures.size());
		CHECK_CLOSE(2.0f, gestures[0].scale, 0.001f);
		CHECK_CLOSE(0.0f, gestures[0].pan.x, 0.001f);
		CHECK_CLOSE(100.0f, gestures[0].center.x, 0.001f);
	}

	TEST(TurningFingersRotates) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { -50, 0 } }, { 2, { 50, 0 } } });

		const auto gestures = feed(tracker, { { 1, { 0, -50 } }, { 2, { 0, 50 } } });

		REQUIRE CHECK_EQUAL(1u, gestures.size());
		CHECK_CLOSE(std::numbers::pi_v<float> / 2, gestures[0].rotation, 0.001f);
		CHECK_CLOSE(1.0f, gestures[0].scale, 0.001f);
	}

	TEST(LiftingAFingerEnds) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } });

		const auto gestures = feed(tracker, { { 1, { 0, 0 } } }, 1000);

		REQUIRE CHECK_EQUAL(1u, gestures.size());
		CHECK(gestures[0].phase == GesturePhase::End);
		CHECK_EQUAL(0, gestures[0].tapFingers);
		CHECK(!tracker.isActive());
	}

	TEST(QuickTwoFingerTouchIsATwoFingerTap) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } } }, 1000);
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } }, 1020);
		CHECK(feed(tracker, { { 2, { 102, 1 } } }, 1120).empty());

		CHECK_EQUAL(2, tapFingersOf(feed(tracker, {}, 1150)));
	}

	TEST(QuickThreeFingerTouchIsAThreeFingerTap) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } }, 1000);
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } }, { 3, { 50, 80 } } }, 1030);
		feed(tracker, { { 3, { 50, 80 } } }, 1150);

		CHECK_EQUAL(3, tapFingersOf(feed(tracker, {}, 1200)));
	}

	TEST(PinchingIsNotATap) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } }, 1000);
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 160, 0 } } }, 1100);

		CHECK_EQUAL(0, tapFingersOf(feed(tracker, {}, 1200)));
	}

	TEST(HoldingTwoFingersIsNotATap) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } }, 1000);

		CHECK_EQUAL(0, tapFingersOf(feed(tracker, {}, 1000 + TouchGestureTracker::kTapMaximumMs + 50)));
	}

	TEST(OneFingerTapIsNotReported) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } } }, 1000);

		CHECK(feed(tracker, {}, 1050).empty());
	}

	TEST(SwappedFingerRebasesWithoutJumping) {
		TouchGestureTracker tracker;
		feed(tracker, { { 1, { 0, 0 } }, { 2, { 100, 0 } } });

		CHECK(feed(tracker, { { 1, { 0, 0 } }, { 3, { 400, 300 } } }).empty());
		const auto gestures = feed(tracker, { { 1, { 0, 0 } }, { 3, { 410, 300 } } });

		REQUIRE CHECK_EQUAL(1u, gestures.size());
		CHECK_CLOSE(5.0f, gestures[0].pan.x, 0.001f);
	}
}

SUITE(TouchGestureCoalescing) {
	TEST(ConsecutiveUpdatesMergeIntoOne) {
		InputCoalescer coalescer;
		coalescer.gesture({ GesturePhase::Begin, { 50, 0 } });
		coalescer.gesture({ GesturePhase::Update, { 60, 0 }, { 10, 0 }, 2.0f, 0.1f });
		coalescer.gesture({ GesturePhase::Update, { 65, 5 }, { 5, 5 }, 1.5f, 0.2f });
		coalescer.gesture({ GesturePhase::End, { 65, 5 } });

		GestureLog log;
		coalescer.deliver(log);

		REQUIRE CHECK_EQUAL(3u, log.gestures.size());
		CHECK(log.gestures[0].phase == GesturePhase::Begin);
		CHECK(log.gestures[1].phase == GesturePhase::Update);
		CHECK_CLOSE(15.0f, log.gestures[1].pan.x, 0.001f);
		CHECK_CLOSE(5.0f, log.gestures[1].pan.y, 0.001f);
		CHECK_CLOSE(3.0f, log.gestures[1].scale, 0.001f);
		CHECK_CLOSE(0.3f, log.gestures[1].rotation, 0.001f);
		CHECK_CLOSE(65.0f, log.gestures[1].center.x, 0.001f);
		CHECK(log.gestures[2].phase == GesturePhase::End);
	}
}

SUITE(PointerPinchConverter) {
	TEST(TotalScaleBecomesIncrementalAndMultipliesBack) {
		tp::PointerPinchConverter converter;
		const tp::TouchGesture begin = converter.begin({ 10, 20 });
		CHECK(begin.phase == tp::GesturePhase::Begin);
		CHECK(begin.center == tp::Vec2F(10.0f, 20.0f));

		tp::halnf product = 1;
		for (tp::halnf total : { 1.2f, 1.5f, 1.5f, 0.9f }) {
			const std::optional<tp::TouchGesture> update = converter.update({ 10, 20 }, { 1, 2 }, total, 0);
			REQUIRE CHECK(update.has_value());
			CHECK(update->pan == tp::Vec2F(1.0f, 2.0f));
			product *= update->scale;
			CHECK_CLOSE(total, product, 0.0001f);
		}

		const std::optional<tp::TouchGesture> end = converter.end({ 10, 20 });
		REQUIRE CHECK(end.has_value());
		CHECK(end->phase == tp::GesturePhase::End);
	}

	TEST(RotationIsConvertedToRadians) {
		tp::PointerPinchConverter converter;
		(void) converter.begin({ 0, 0 });
		const std::optional<tp::TouchGesture> update = converter.update({ 0, 0 }, { 0, 0 }, 1, 90);
		REQUIRE CHECK(update.has_value());
		CHECK_CLOSE(1.5707963f, update->rotation, 0.0001f);
	}

	TEST(UpdatesOutsideAGestureAreIgnoredAndNewGestureRestartsScale) {
		tp::PointerPinchConverter converter;
		CHECK(!converter.update({ 0, 0 }, { 0, 0 }, 2, 0).has_value());
		CHECK(!converter.end({ 0, 0 }).has_value());

		(void) converter.begin({ 0, 0 });
		(void) converter.update({ 0, 0 }, { 0, 0 }, 2, 0);
		(void) converter.end({ 0, 0 });
		(void) converter.begin({ 0, 0 });
		const std::optional<tp::TouchGesture> fresh = converter.update({ 0, 0 }, { 0, 0 }, 1.25f, 0);
		REQUIRE CHECK(fresh.has_value());
		CHECK_CLOSE(1.25f, fresh->scale, 0.0001f);
	}

	TEST(ZeroScaleFromCompositorIsTreatedAsNoChange) {
		tp::PointerPinchConverter converter;
		(void) converter.begin({ 0, 0 });
		const std::optional<tp::TouchGesture> zero = converter.update({ 0, 0 }, { 0, 0 }, 0, 0);
		REQUIRE CHECK(zero.has_value());
		CHECK_CLOSE(1, zero->scale, 0.0001f);
		const std::optional<tp::TouchGesture> next = converter.update({ 0, 0 }, { 0, 0 }, 2, 0);
		CHECK_CLOSE(2, next->scale, 0.0001f);
	}
}
