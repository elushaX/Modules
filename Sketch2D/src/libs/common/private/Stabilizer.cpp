#include "Stabilizer.hpp"
#include "Profiler.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	namespace {
		constexpr halnf kEmitDistance = 0.75f;
		constexpr halnf kSettledDistance = 0.05f;
		constexpr halnf kFastStrokeShare = 0.35f;
		constexpr halnf kFastCurvatureLength = 120.0f;

		Vec2F lerp(Vec2F from, Vec2F to, halnf amount) { return from + (to - from) * amount; }
		halnf lerp(halnf from, halnf to, halnf amount) { return from + (to - from) * amount; }
		halnf distance(Vec2F a, Vec2F b) { return (halnf) (b - a).length(); }
	}

	halnf Stabilizer::smoothingLength(halnf amount) {
		if (amount <= 0.0f) return 0;
		return 100.0f * std::pow(amount, 1.6f);
	}

	halnf Stabilizer::ropeLength(halnf amount) {
		if (amount <= 0.0f) return 0;
		return 6.0f * amount;
	}

	halnf Stabilizer::catchUpMs(halnf amount) {
		if (amount <= 0.0f) return 0;
		return 20.0f + 160.0f * amount;
	}

	void Stabilizer::begin(const StrokePoint& point, halnf amount) {
		mActive = true;
		mAmount = std::clamp(amount, 0.0f, 1.0f);
		mIdleMs = 0;
		mPen = point;
		mAnchor = point;
		mFirstStage = point;
		mSmoothed = point;
		mEmitted = point;
		mDirection = { 0, 0 };
		mStraightness = 0;
		mOutput.clear();
		mOutput.push_back(point);
	}

	void Stabilizer::moveTo(const StrokePoint& point) {
		TP_PROFILE_SCOPE("stroke.stabilizer");
		if (!mActive) return;

		mIdleMs = 0;
		if (mAmount <= 0.0f) {
			mPen = point;
			mAnchor = point;
			mFirstStage = point;
			mSmoothed = point;
			emit();
			return;
		}

		const StrokePoint from = mPen;
		const halnf length = distance(from.position, point.position);
		const int4 steps = std::max(1, (int4) std::ceil(length / kStepLength));
		for (int4 index = 1; index <= steps; index++) {
			const halnf amount = (halnf) index / (halnf) steps;
			step(
				{ lerp(from.position, point.position, amount), lerp(from.pressure, point.pressure, amount) },
				length / (halnf) steps
			);
		}
		mPen = point;
	}

	void Stabilizer::step(const StrokePoint& target, halnf travelled) {
		const halnf rope = ropeLength(mAmount);
		const Vec2F offset = target.position - mAnchor.position;
		const halnf reach = (halnf) offset.length();
		if (reach > rope) {
			const Vec2F moved = offset * ((reach - rope) / reach);
			mAnchor.position += moved;
			trackDirection(moved);
		}
		mAnchor.pressure = target.pressure;

		const halnf length = smoothingLength(mAmount) * (1.0f - kFastStrokeShare * mStraightness);
		follow(1.0f - std::exp(-2.0f * travelled / length));
		emit();
	}

	void Stabilizer::trackDirection(Vec2F moved) {
		const halnf length = (halnf) moved.length();
		if (length <= 0.0f) return;
		const Vec2F unit = moved / length;
		const halnf memory = std::exp(-length / kFastCurvatureLength);
		const halnf agreement = mDirection.x * unit.x + mDirection.y * unit.y;
		mStraightness = mStraightness * memory + std::max(0.0f, agreement) * (1.0f - memory);
		mDirection = mDirection * memory + unit * (1.0f - memory);
		const halnf directionLength = (halnf) mDirection.length();
		if (directionLength > 0.0f) mDirection = mDirection / directionLength;
	}

	void Stabilizer::follow(halnf amount) {
		mFirstStage.position = lerp(mFirstStage.position, mAnchor.position, amount);
		mFirstStage.pressure = lerp(mFirstStage.pressure, mAnchor.pressure, amount);
		mSmoothed.position = lerp(mSmoothed.position, mFirstStage.position, amount);
		mSmoothed.pressure = lerp(mSmoothed.pressure, mFirstStage.pressure, amount);
	}

	void Stabilizer::advance(halnf deltaMs) {
		TP_PROFILE_SCOPE("stroke.stabilizer");
		if (!mActive || mAmount <= 0.0f || deltaMs <= 0.0f) return;

		mIdleMs += deltaMs;
		if (mIdleMs < kIdleBeforeCatchUpMs) return;

		mAnchor = mPen;
		follow(1.0f - std::exp(-2.0f * deltaMs / catchUpMs(mAmount)));
		if (isSettled()) {
			mFirstStage = mPen;
			mSmoothed = mPen;
			if (distance(mEmitted.position, mSmoothed.position) > 0.0f) {
				mEmitted = mSmoothed;
				mOutput.push_back(mSmoothed);
			}
			return;
		}
		emit();
	}

	void Stabilizer::finish() {
		if (!mActive) return;
		if (distance(mEmitted.position, mSmoothed.position) > 0.0f) mOutput.push_back(mSmoothed);
		mActive = false;
	}

	void Stabilizer::emit() {
		if (distance(mEmitted.position, mSmoothed.position) < kEmitDistance) return;
		mEmitted = mSmoothed;
		mOutput.push_back(mSmoothed);
	}

	bool Stabilizer::isSettled() const { return distance(mSmoothed.position, mPen.position) < kSettledDistance; }

	std::vector<StrokePoint> Stabilizer::takeOutput() {
		std::vector<StrokePoint> output;
		output.swap(mOutput);
		return output;
	}
}
