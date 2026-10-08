#include "ScrollPhysics.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>
#include <vector>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kFrameMs = 16;

	std::vector<halnf> settle(MagneticStep& step) {
		std::vector<halnf> offsets;
		for (int frame = 0; frame < 400 && step.isActive(); frame++) {
			step.advance(kFrameMs);
			offsets.push_back(step.offset());
		}
		return offsets;
	}

	void hold(MagneticStep& step, halnf pull) {
		step.push(pull);
		step.hold();
		for (int frame = 0; frame < 120; frame++)
			step.advance(kFrameMs);
	}
}

SUITE(MagneticStep) {
	TEST(ResistanceGrowsWithThePullAndNeverReachesTheGive) {
		const halnf give = MagneticStepTuning::standard().give;
		CHECK_EQUAL(0.0f, MagneticStep::resisted(0, give));
		CHECK_CLOSE(0.1f, MagneticStep::resisted(0.1f, 100), 0.001f);

		const halnf small = MagneticStep::resisted(0.1f, give);
		const halnf large = MagneticStep::resisted(0.4f, give);
		CHECK(small > 0);
		CHECK(large > small);
		CHECK(large - MagneticStep::resisted(0.3f, give) < small);
		CHECK(MagneticStep::resisted(100, give) <= give);
		CHECK_CLOSE(-large, MagneticStep::resisted(-0.4f, give), 1e-6f);
	}

	TEST(TheFirstDeltaMovesTheOffsetWithoutADeadZone) {
		MagneticStep step;
		CHECK_EQUAL(0, step.push(0.01f));
		step.advance(kFrameMs);
		CHECK(step.offset() > 0);
		CHECK(step.isPulling());
		CHECK_EQUAL(0, step.committed());
	}

	TEST(APullBelowTheThresholdResistsAndSpringsBackAfterTheQuietDelay) {
		MagneticStep step;
		step.push(0.3f);
		step.advance(kFrameMs);
		const halnf early = step.offset();
		CHECK(early > 0);
		CHECK(early < step.pulledOffset());
		CHECK_CLOSE(MagneticStep::resisted(0.3f, step.tuning().give), step.pulledOffset(), 1e-6f);
		CHECK(step.pulledOffset() < 0.3f);
		CHECK(step.isPulling());

		bool released = false;
		for (halnf waited = 0; waited < step.tuning().releaseDelayMs + kFrameMs; waited += kFrameMs)
			released = step.advance(kFrameMs).released || released;
		CHECK(released);
		CHECK(!step.isPulling());

		const std::vector<halnf> offsets = settle(step);
		CHECK(!step.isActive());
		CHECK_EQUAL(0.0f, step.offset());
		for (halnf offset : offsets)
			CHECK(offset >= -0.0005f);
		CHECK_EQUAL(0, step.committed());
	}

	TEST(InputKeepsTheReleaseCountdownFromRunningOut) {
		MagneticStep step;
		for (int frame = 0; frame < 40; frame++) {
			step.push(0.002f);
			CHECK(!step.advance(kFrameMs).released);
		}
		CHECK(step.isPulling());
	}

	TEST(HoldingStopsTheCountdownUntilReleased) {
		MagneticStep step;
		step.push(0.2f);
		step.hold();
		for (int frame = 0; frame < 60; frame++)
			CHECK(!step.advance(kFrameMs).released);
		CHECK(step.offset() > 0.1f);

		step.release();
		settle(step);
		CHECK_EQUAL(0.0f, step.offset());
	}

	TEST(CrossingTheThresholdCommitsAStepWhoseVisiblePositionStaysContinuous) {
		MagneticStep step;
		hold(step, 0.45f);
		const halnf before = step.position();

		CHECK_EQUAL(1, step.push(0.1f));
		CHECK_EQUAL(1, step.committed());
		CHECK_EQUAL(0.0f, step.pulled());
		CHECK_CLOSE(before, step.position(), 1e-5f);
		CHECK(!step.isPulling());

		halnf previous = step.position();
		for (halnf offset : settle(step)) {
			const halnf position = 1 + offset;
			CHECK(position >= previous - 1e-5f);
			CHECK(position <= 1.0005f);
			previous = position;
		}
		CHECK_EQUAL(1.0f, step.position());
	}

	TEST(AFastPullThatCommitsNeverOvershootsTheNewStep) {
		for (const halnf delta : { 0.1f, 0.15f, 0.2f, 0.3f, 0.45f }) {
			MagneticStep step;
			while (step.committed() == 0) {
				step.push(delta);
				step.advance(kFrameMs);
			}
			for (halnf offset : settle(step))
				CHECK(offset <= 0.0005f);
			CHECK_EQUAL(1.0f, step.position());
		}
	}

	TEST(OneLongGestureCommitsSeveralStepsInEitherDirection) {
		MagneticStep step;
		int forward = 0;
		for (int frame = 0; frame < 20; frame++) {
			forward += step.push(0.2f);
			step.advance(kFrameMs);
		}
		CHECK_EQUAL(6, forward);
		CHECK_EQUAL((long) forward, step.committed());

		int backward = 0;
		for (int frame = 0; frame < 20; frame++) {
			backward += step.push(-0.2f);
			step.advance(kFrameMs);
		}
		CHECK_EQUAL(-6, backward);
		CHECK_EQUAL((long) (forward + backward), step.committed());
	}

	TEST(MovingByStepsSlidesTheOffsetFromTheOldStepWithoutOvershoot) {
		MagneticStep step;
		step.moveBy(2);
		CHECK_EQUAL(2, step.committed());
		CHECK_CLOSE(0.0f, step.position(), 1e-6f);
		CHECK(step.isMoving());

		halnf previous = step.position();
		for (halnf offset : settle(step)) {
			const halnf position = 2 + offset;
			CHECK(position >= previous - 1e-5f);
			CHECK(position <= 2.0005f);
			previous = position;
		}
		CHECK_EQUAL(2.0f, step.position());
	}

	TEST(MovesAddUpWhileTheSlideIsStillRunning) {
		MagneticStep step;
		step.moveBy(1);
		step.advance(kFrameMs);
		step.moveBy(1);
		CHECK_EQUAL(2, step.committed());
		settle(step);
		CHECK_EQUAL(2.0f, step.position());
	}

	TEST(JumpingResetsPullAndMotion) {
		MagneticStep step;
		step.push(0.3f);
		step.moveBy(3);
		step.jumpTo(-1);
		CHECK_EQUAL(-1, step.committed());
		CHECK_EQUAL(-1.0f, step.position());
		CHECK(!step.isActive());
	}

	TEST(TuningIsReadLiveFromItsOwner) {
		MagneticStepTuning tuning;
		MagneticStep step(tuning);
		CHECK_EQUAL(0, step.push(0.3f));

		tuning.commitFraction = 0.3f;
		CHECK_EQUAL(1, step.push(0.05f));
	}
}
