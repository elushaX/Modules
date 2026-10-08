#include "ScrollPhysics.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr double kGravityEarth = 9.80665;
	constexpr double kInchesPerMeter = 39.37;
	constexpr double kLookAndFeel = 0.84;
	const double kDecelerationRate = std::log(0.78) / std::log(0.9);

	struct SplineTable {
		std::array<halnf, FlingSpline::kSamples + 1> position{};

		SplineTable() {
			constexpr halnf p1 = FlingSpline::kStartTension * FlingSpline::kInflexion;
			constexpr halnf p2 = 1.0f - FlingSpline::kEndTension * (1.0f - FlingSpline::kInflexion);
			halnf xMinimum = 0;
			for (int4 index = 0; index < FlingSpline::kSamples; index++) {
				const halnf alpha = (halnf) index / FlingSpline::kSamples;
				halnf xMaximum = 1;
				halnf x = 0;
				halnf coefficient = 0;
				while (true) {
					x = xMinimum + (xMaximum - xMinimum) / 2.0f;
					coefficient = 3.0f * x * (1.0f - x);
					const halnf tx = coefficient * ((1.0f - x) * p1 + x * p2) + x * x * x;
					if (std::abs(tx - alpha) < 1e-5f) break;
					if (tx > alpha) xMaximum = x;
					else xMinimum = x;
				}
				position[(size_t) index] = coefficient * ((1.0f - x) * FlingSpline::kStartTension + x) + x * x * x;
			}
			position[FlingSpline::kSamples] = 1.0f;
		}
	};

	const SplineTable& table() {
		static const SplineTable instance;
		return instance;
	}
}

FlingSpline::FlingSpline(halnf pixelsPerInch) :
	mPhysicalCoefficient((halnf) (kGravityEarth * kInchesPerMeter * pixelsPerInch * kLookAndFeel)) {}

double FlingSpline::deceleration(halnf velocityPerSecond) const {
	return std::log(kInflexion * std::abs(velocityPerSecond) / (kScrollFriction * mPhysicalCoefficient));
}

halnf FlingSpline::distanceFor(halnf velocityPerSecond) const {
	if (velocityPerSecond == 0) return 0;
	const double rate = kDecelerationRate;
	return (halnf) (kScrollFriction * mPhysicalCoefficient *
									std::exp(rate / (rate - 1.0) * deceleration(velocityPerSecond)));
}

halnf FlingSpline::durationFor(halnf velocityPerSecond) const {
	if (velocityPerSecond == 0) return 0;
	return (halnf) (1000.0 * std::exp(deceleration(velocityPerSecond) / (kDecelerationRate - 1.0)));
}

halnf FlingSpline::splinePosition(halnf time) {
	const halnf t = std::clamp(time, 0.0f, 1.0f);
	const int4 index = (int4) (kSamples * t);
	if (index >= kSamples) return 1.0f;
	const halnf low = (halnf) index / kSamples;
	const halnf high = (halnf) (index + 1) / kSamples;
	const halnf lowValue = table().position[(size_t) index];
	const halnf highValue = table().position[(size_t) index + 1];
	return lowValue + (t - low) * (highValue - lowValue) / (high - low);
}

void FlingSpline::start(halnf position, halnf velocityPerSecond, halnf minimum, halnf maximum) {
	mStart = mPosition = position;
	mVelocity = velocityPerSecond;
	mElapsedMs = 0;
	mHitEdge = false;
	mSplineDurationMs = mDurationMs = durationFor(velocityPerSecond);
	mSplineDistance = distanceFor(velocityPerSecond) * (velocityPerSecond < 0 ? -1.0f : 1.0f);
	mFinal = mStart + mSplineDistance;
	mActive = velocityPerSecond != 0 && mDurationMs > 0;

	const auto clampAt = [this](halnf edge) {
		const halnf wanted = (edge - mStart) / mSplineDistance;
		halnf low = 0;
		halnf high = 1;
		for (int4 step = 0; step < 40; step++) {
			const halnf middle = (low + high) / 2;
			if (splinePosition(middle) < wanted) low = middle;
			else high = middle;
		}
		mDurationMs = mSplineDurationMs * high;
		mFinal = edge;
		mHitEdge = true;
	};
	if (mActive && mFinal < minimum) clampAt(minimum);
	if (mActive && mFinal > maximum) clampAt(maximum);
}

bool FlingSpline::advance(halnf deltaMs) {
	if (!mActive) return false;
	mElapsedMs += deltaMs;
	if (mElapsedMs >= mDurationMs) {
		const halnf t = mDurationMs / mSplineDurationMs;
		const halnf step = 1.0f / kSamples;
		mVelocity = (splinePosition(std::min(1.0f, t + step)) - splinePosition(std::max(0.0f, t - step))) / (2 * step) *
								mSplineDistance / mSplineDurationMs * 1000.0f;
		if (!mHitEdge) mVelocity = 0;
		mPosition = mFinal;
		mActive = false;
		return false;
	}
	const halnf t = mElapsedMs / mSplineDurationMs;
	const halnf step = 1.0f / kSamples;
	mPosition = mStart + splinePosition(t) * mSplineDistance;
	mVelocity = (splinePosition(std::min(1.0f, t + step)) - splinePosition(std::max(0.0f, t - step))) / (2 * step) *
							mSplineDistance / mSplineDurationMs * 1000.0f;
	return true;
}

halnf StretchEffect::pull(halnf deltaPixels) {
	if (!mPulling) mPullDistance = mDistance;
	mPulling = true;
	const halnf before = mPullDistance;
	mPullDistance = std::clamp(mPullDistance + deltaPixels / mLength, 0.0f, 1.0f);
	mDistance = mPullDistance;
	mVelocity = 0;
	return (mPullDistance - before) * mLength;
}

void StretchEffect::release() {
	mPulling = false;
	mPullDistance = 0;
	mVelocity = 0;
}

void StretchEffect::absorb(halnf velocityPerSecond) {
	mPulling = false;
	mVelocity = std::abs(velocityPerSecond) * kAbsorbVelocityAdjustment;
}

void StretchEffect::finish() {
	mPulling = false;
	mDistance = mPullDistance = 0;
	mVelocity = 0;
}

bool StretchEffect::advance(halnf deltaMs) {
	if (mPulling || !isActive()) return false;
	const double seconds = deltaMs / 1000.0;
	const double damped = kNaturalFrequency * std::sqrt(1 - kDampingRatio * kDampingRatio);
	const double cosine = (double) mDistance * mLength;
	const double sine = (1 / damped) * (kDampingRatio * kNaturalFrequency * mDistance * mLength + mVelocity);
	const double decay = std::exp(-kDampingRatio * kNaturalFrequency * seconds);
	const double displacement = decay * (cosine * std::cos(damped * seconds) + sine * std::sin(damped * seconds));
	const double velocity =
		displacement * (-kNaturalFrequency) * kDampingRatio +
		decay * (-damped * cosine * std::sin(damped * seconds) + damped * sine * std::cos(damped * seconds));
	mDistance = (halnf) (displacement / mLength);
	mVelocity = (halnf) velocity;
	if (mDistance > 1) {
		mDistance = 1;
		mVelocity = 0;
	}
	if (displacement < 0 || (std::abs(velocity) < kVelocityThreshold && displacement < kValueThreshold)) {
		mDistance = 0;
		mVelocity = 0;
		return false;
	}
	return true;
}

halnf StretchEffect::stretchIntensity(halnf distance) {
	const halnf amount = std::clamp(std::abs(distance), 0.0f, 1.0f);
	const halnf linear = kLinearIntensity * amount;
	const double scalar = 2.718281828 / kExponentialReach;
	const halnf exponential = (halnf) (kExponentialIntensity * (1 - std::exp(-amount * scalar)));
	return linear + exponential;
}

halnf StretchEffect::visualOffset() const { return stretchIntensity(mDistance) * mLength; }

const MagneticStepTuning& MagneticStepTuning::standard() {
	static const MagneticStepTuning tuning;
	return tuning;
}

MagneticStep::MagneticStep(const MagneticStepTuning& tuning) :
	mTuning(tuning),
	mOffset(0, tuning.spring, kSettleTolerance) {}

void MagneticStep::springTo(halnf target) {
	mOffset.setParameters(mTuning.spring);
	mOffset.animateTo(target);
}

halnf MagneticStep::resisted(halnf pull, halnf give) {
	if (pull == 0 || give <= 0) return 0;
	const halnf magnitude = give * (1 - std::exp(-std::abs(pull) / give));
	return pull > 0 ? magnitude : -magnitude;
}

int MagneticStep::push(halnf steps) {
	if (steps == 0) return 0;
	mPull += steps;

	if (std::abs(mPull) >= mTuning.commitFraction) {
		const int direction = mPull > 0 ? 1 : -1;
		moveBy(direction);
		return direction;
	}

	springTo(pulledOffset());
	mReleaseInMs = mTuning.releaseDelayMs;
	return 0;
}

void MagneticStep::moveBy(long steps) {
	mPull = 0;
	mReleaseInMs.reset();
	mCommitted += steps;
	mOffset.setParameters(mTuning.spring);
	const halnf shown = mOffset.value() - halnf(steps);
	mOffset.launch(shown, velocityWithoutOvershoot(shown, mOffset.velocity()), 0);
}

halnf MagneticStep::velocityWithoutOvershoot(halnf offset, halnf velocity) const {
	const halnf towardRest = -offset * velocity;
	if (towardRest <= 0) return velocity;

	const halnf limit = std::abs(offset) * std::sqrt(mTuning.spring.stiffness);
	return std::clamp(velocity, -limit, limit);
}

void MagneticStep::release() {
	mPull = 0;
	mReleaseInMs.reset();
	springTo(0);
}

void MagneticStep::stop() {
	mPull = 0;
	mReleaseInMs.reset();
	mOffset.jumpTo(0);
}

void MagneticStep::jumpTo(long step) {
	stop();
	mCommitted = step;
}

MagneticStep::Tick MagneticStep::advance(halnf deltaMs) {
	Tick tick;
	if (mReleaseInMs) {
		*mReleaseInMs -= deltaMs;
		if (*mReleaseInMs <= 0) {
			release();
			tick.released = true;
		}
	}

	tick.moving = mOffset.advance(deltaMs);
	return tick;
}
