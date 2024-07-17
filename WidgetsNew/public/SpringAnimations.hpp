#pragma once

#include "Color.hpp"
#include "Rect.hpp"
#include "Timing.hpp"

namespace tp {
	class SpringValue {
	public:
		SpringValue() = default;

		void updateCurrentPosition() {
			const auto deltaTime = halnf(gCurrentTime - mPrevTime);
			const auto deltaPos = mTargetPosition - mCurrentPosition;

			auto deltaVelocity = deltaPos * mSpringStiffness;
			deltaVelocity -= mVelocity * (mVelocityDamping);

			mVelocity += deltaVelocity;
			mCurrentPosition += mVelocity * deltaTime;
			mPrevTime = gCurrentTime;
		}

		void setTargetPosition(halnf pos) {
			mTargetPosition = pos;
			if (mVelocity == 0) mPrevTime = gCurrentTime;
		}

		[[nodiscard]] halnf getCurrentPos() const { return mCurrentPosition; }
		[[nodiscard]] halnf getTargetPos() const { return mTargetPosition; }
		[[nodiscard]] halnf getVelocity() const { return mVelocity; }
		[[nodiscard]] halnf getDeltaPos() const { return mTargetPosition - mCurrentPosition; }

		void endAnimation() {
			mCurrentPosition = mTargetPosition;
			mVelocity = 0;
		}

	private:
		time_ms mPrevTime = 0;

		halnf mTargetPosition = 0;
		halnf mCurrentPosition = 0;

		halnf mVelocity = 0;

		halnf mSpringStiffness = 0.0041;
		halnf mVelocityDamping = 0.25f;
	};

	class SpringVec {
	public:
		SpringVec() = default;

		void updateCurrentPosition() {
			mPosX.updateCurrentPosition();
			mPosY.updateCurrentPosition();
		}

		void setTargetPosition(Vec2F pos) {
			mPosX.setTargetPosition(pos.x);
			mPosY.setTargetPosition(pos.y);
		}

		[[nodiscard]] Vec2F getCurrentPos() const {
			return { mPosX.getCurrentPos(), mPosY.getCurrentPos() };
		}

		[[nodiscard]] Vec2F getTargetPos() const {
			return { mPosX.getTargetPos(), mPosY.getTargetPos() };
		}

		[[nodiscard]] Vec2F getVelocity() const {
			return { mPosX.getVelocity(), mPosY.getVelocity() };
		}

		[[nodiscard]] bool checkAnimationShouldEnd() const {
			halnf velocityEpsilon = 0.0001f;
			halnf positionEpsilon = 1;

			const auto vel = abs(mPosY.getVelocity()) > velocityEpsilon || abs(mPosX.getVelocity()) > velocityEpsilon;
			const auto pos = abs(mPosY.getDeltaPos()) > positionEpsilon || abs(mPosX.getDeltaPos()) > positionEpsilon;

			return !(pos || vel);
		}

		void endAnimation() {
			mPosX.endAnimation();
			mPosY.endAnimation();
		}

	private:
		SpringValue mPosY;
		SpringValue mPosX;
	};

	class SpringRect {
	public:
		SpringRect() = default;

		[[nodiscard]] RectF getTargetRect() const {
			const auto start = mStartPos.getTargetPos();
			const auto end = mEndPos.getTargetPos();
			return { start, end - start };
		}

		[[nodiscard]] RectF getCurrentRect() const {
			const auto start = mStartPos.getCurrentPos();
			const auto end = mEndPos.getCurrentPos();
			return { start, end - start };
		}

		[[nodiscard]] RGBA getCurrentColor() const {
			const auto start = mStartPos.getCurrentPos();
			const auto end = mEndPos.getCurrentPos();
			return { start.x, start.y, end.x, end.y };
		}

		void setTargetRect(const RectF& rect) {
			mStartPos.setTargetPosition(rect.p1());
			mEndPos.setTargetPosition(rect.p3());
		}

		void setTargetColor(const RGBA& color) {
			mStartPos.setTargetPosition({ color.r, color.g });
			mEndPos.setTargetPosition({ color.b, color.a });
		}

		SpringVec& getStart() {
			return mStartPos;
		}

		SpringVec& getEnd() {
			return mEndPos;
		}

		void updateCurrentRect() {
			mStartPos.updateCurrentPosition();
			mEndPos.updateCurrentPosition();
		}

		[[nodiscard]] bool checkAnimationShouldEnd() const {
			return mStartPos.checkAnimationShouldEnd() && mEndPos.checkAnimationShouldEnd();
		}

		void endAnimation() {
			mStartPos.endAnimation();
			mEndPos.endAnimation();
		}

	private:
		SpringVec mStartPos;
		SpringVec mEndPos;
	};
}