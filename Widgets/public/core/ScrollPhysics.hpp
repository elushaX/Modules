#pragma once

#include "Animation.hpp"
#include "Environment.hpp"

#include <optional>

namespace tp::ui {

	class FlingSpline {
	public:
		static constexpr halnf kScrollFriction = 0.015f;
		static constexpr halnf kInflexion = 0.35f;
		static constexpr halnf kStartTension = 0.5f;
		static constexpr halnf kEndTension = 1.0f;
		static constexpr int4 kSamples = 100;

		explicit FlingSpline(halnf pixelsPerInch = 160.0f);

		void start(halnf position, halnf velocityPerSecond, halnf minimum, halnf maximum);
		void stop() { mActive = false; }
		bool advance(halnf deltaMs);

		[[nodiscard]] bool isActive() const { return mActive; }
		[[nodiscard]] halnf position() const { return mPosition; }
		[[nodiscard]] halnf velocity() const { return mVelocity; }
		[[nodiscard]] halnf finalPosition() const { return mFinal; }
		[[nodiscard]] halnf durationMs() const { return mDurationMs; }
		[[nodiscard]] bool hitEdge() const { return mHitEdge; }

		[[nodiscard]] halnf distanceFor(halnf velocityPerSecond) const;
		[[nodiscard]] halnf durationFor(halnf velocityPerSecond) const;
		[[nodiscard]] static halnf splinePosition(halnf time);

	private:
		[[nodiscard]] double deceleration(halnf velocityPerSecond) const;

		halnf mPhysicalCoefficient;
		halnf mStart = 0;
		halnf mFinal = 0;
		halnf mPosition = 0;
		halnf mVelocity = 0;
		halnf mSplineDistance = 0;
		halnf mSplineDurationMs = 0;
		halnf mDurationMs = 0;
		halnf mElapsedMs = 0;
		bool mActive = false;
		bool mHitEdge = false;
	};

	class StretchEffect {
	public:
		static constexpr double kNaturalFrequency = 24.657;
		static constexpr double kDampingRatio = 0.98;
		static constexpr halnf kAbsorbVelocityAdjustment = 13.0f;
		static constexpr halnf kLinearIntensity = 0.016f;
		static constexpr halnf kExponentialIntensity = 0.016f;
		static constexpr halnf kExponentialReach = 0.33f;
		static constexpr double kVelocityThreshold = 0.01;
		static constexpr double kValueThreshold = 0.001;

		void setLength(halnf length) { mLength = length > 1 ? length : 1; }
		halnf pull(halnf deltaPixels);
		void release();
		void absorb(halnf velocityPerSecond);
		void finish();
		bool advance(halnf deltaMs);

		[[nodiscard]] halnf distance() const { return mDistance; }
		[[nodiscard]] bool isPulling() const { return mPulling; }
		[[nodiscard]] bool isActive() const { return mDistance > 0 || mVelocity != 0; }
		[[nodiscard]] halnf visualOffset() const;
		[[nodiscard]] static halnf stretchIntensity(halnf distance);

	private:
		halnf mLength = 1;
		halnf mDistance = 0;
		halnf mPullDistance = 0;
		halnf mVelocity = 0;
		bool mPulling = false;
	};

	struct MagneticStepTuning {
		halnf commitFraction = 0.5f;
		halnf give = 0.3f;
		halnf releaseDelayMs = 160;
		SpringParameters spring = Motion::smooth();

		[[nodiscard]] static const MagneticStepTuning& standard();
	};

	class MagneticStep {
	public:
		static constexpr halnf kSettleTolerance = 0.0005f;

		struct Tick {
			bool released = false;
			bool moving = false;
		};

		explicit MagneticStep(const MagneticStepTuning& tuning = MagneticStepTuning::standard());

		[[nodiscard]] const MagneticStepTuning& tuning() const { return mTuning; }

		int push(halnf steps);
		void moveBy(long steps);
		void hold() { mReleaseInMs.reset(); }
		void release();
		void stop();
		void jumpTo(long step);
		Tick advance(halnf deltaMs);

		[[nodiscard]] long committed() const { return mCommitted; }
		[[nodiscard]] halnf pulled() const { return mPull; }
		[[nodiscard]] halnf pulledOffset() const { return resisted(mPull, mTuning.give); }
		[[nodiscard]] halnf offset() const { return mOffset.value(); }
		[[nodiscard]] halnf position() const { return halnf(mCommitted) + offset(); }
		[[nodiscard]] bool isPulling() const { return mPull != 0 || mReleaseInMs.has_value(); }
		[[nodiscard]] bool isMoving() const { return mOffset.isAnimating(); }
		[[nodiscard]] bool isActive() const { return isPulling() || isMoving(); }

		[[nodiscard]] static halnf resisted(halnf pull, halnf give);

	private:
		void springTo(halnf target);
		[[nodiscard]] halnf velocityWithoutOvershoot(halnf offset, halnf velocity) const;

		const MagneticStepTuning& mTuning;
		AnimatedFloat mOffset;
		long mCommitted = 0;
		halnf mPull = 0;
		std::optional<halnf> mReleaseInMs;
	};
}
