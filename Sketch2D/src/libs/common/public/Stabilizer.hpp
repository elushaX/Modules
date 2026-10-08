#pragma once

#include "Environment.hpp"
#include "Vec.hpp"

#include <vector>

namespace tp::sketch2d {

	struct StrokePoint {
		Vec2F position = { 0, 0 };
		halnf pressure = 1;
	};

	class Stabilizer {
	public:
		static constexpr halnf kDefaultAmount = 0.325f;
		static constexpr halnf kStepLength = 0.5f;
		static constexpr halnf kIdleBeforeCatchUpMs = 40;

		void begin(const StrokePoint& point, halnf amount);
		void moveTo(const StrokePoint& point);
		void advance(halnf deltaMs);
		void finish();

		[[nodiscard]] std::vector<StrokePoint> takeOutput();

		[[nodiscard]] bool isActive() const { return mActive; }
		[[nodiscard]] bool isSmoothing() const { return mAmount > 0; }
		[[nodiscard]] halnf amount() const { return mAmount; }
		[[nodiscard]] const StrokePoint& pen() const { return mPen; }
		[[nodiscard]] const StrokePoint& smoothed() const { return mSmoothed; }
		[[nodiscard]] bool isSettled() const;

		[[nodiscard]] static halnf smoothingLength(halnf amount);
		[[nodiscard]] static halnf ropeLength(halnf amount);
		[[nodiscard]] static halnf catchUpMs(halnf amount);

	private:
		void step(const StrokePoint& target, halnf travelled);
		void follow(halnf amount);
		void trackDirection(Vec2F moved);
		void emit();

		bool mActive = false;
		halnf mAmount = 0;
		halnf mIdleMs = 0;
		StrokePoint mPen;
		StrokePoint mAnchor;
		StrokePoint mFirstStage;
		StrokePoint mSmoothed;
		StrokePoint mEmitted;
		Vec2F mDirection = { 0, 0 };
		halnf mStraightness = 0;
		std::vector<StrokePoint> mOutput;
	};
}
