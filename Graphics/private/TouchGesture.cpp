#include "TouchGesture.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace tp {

	namespace {
		constexpr halnf kMinimumSpan = 1.0f;

		halnf wrapAngle(halnf angle) {
			constexpr halnf pi = std::numbers::pi_v<halnf>;
			while (angle > pi)
				angle -= 2 * pi;
			while (angle < -pi)
				angle += 2 * pi;
			return angle;
		}
	}

	void TouchGestureTracker::track(const TouchPoint& first, const TouchPoint& second) {
		const Vec2F offset = second.position - first.position;
		mFirst = first.id;
		mSecond = second.id;
		mCenter = (first.position + second.position) * 0.5f;
		mDistance = std::sqrt(offset.x * offset.x + offset.y * offset.y);
		mAngle = std::atan2(offset.y, offset.x);
	}

	void TouchGestureTracker::followTap(const std::vector<TouchPoint>& touches, uint32_t timeMs) {
		if (mTapStarts.empty() && !touches.empty()) {
			mTapStartMs = timeMs;
			mTapStillPossible = true;
		}
		for (const TouchPoint& touch : touches) {
			auto start = std::find_if(mTapStarts.begin(), mTapStarts.end(), [&](const TouchPoint& known) {
				return known.id == touch.id;
			});
			if (start == mTapStarts.end()) {
				mTapStarts.push_back(touch);
				continue;
			}
			const Vec2F travel = touch.position - start->position;
			if (travel.x * travel.x + travel.y * travel.y > kTapMaximumTravel * kTapMaximumTravel) mTapStillPossible = false;
		}
	}

	int32_t TouchGestureTracker::finishedTapFingers(uint32_t timeMs) const {
		const bool quick = timeMs - mTapStartMs <= kTapMaximumMs;
		return mTapStillPossible && quick && mTapStarts.size() >= 2 ? (int32_t) mTapStarts.size() : 0;
	}

	void TouchGestureTracker::update(
		const std::vector<TouchPoint>& touches, std::vector<TouchGesture>& gestures, uint32_t timeMs
	) {
		followTap(touches, timeMs);
		if (touches.empty()) {
			const int32_t tapFingers = finishedTapFingers(timeMs);
			mTapStarts.clear();
			cancel(gestures);
			if (!gestures.empty() && gestures.back().phase == GesturePhase::End) gestures.back().tapFingers = tapFingers;
			return;
		}
		if (touches.size() < 2) {
			if (finishedTapFingers(timeMs) == 0) cancel(gestures);
			return;
		}

		const TouchPoint& first = touches[0];
		const TouchPoint& second = touches[1];

		if (!mActive) {
			track(first, second);
			mActive = true;
			gestures.push_back({ GesturePhase::Begin, mCenter });
			return;
		}

		if (first.id != mFirst || second.id != mSecond) {
			track(first, second);
			return;
		}

		const Vec2F previousCenter = mCenter;
		const halnf previousDistance = mDistance;
		const halnf previousAngle = mAngle;
		track(first, second);

		TouchGesture gesture;
		gesture.phase = GesturePhase::Update;
		gesture.center = mCenter;
		gesture.pan = mCenter - previousCenter;
		gesture.scale = previousDistance >= kMinimumSpan && mDistance >= kMinimumSpan ? mDistance / previousDistance : 1;
		gesture.rotation = wrapAngle(mAngle - previousAngle);
		gestures.push_back(gesture);
	}

	void TouchGestureTracker::cancel(std::vector<TouchGesture>& gestures) {
		if (!mActive) return;
		mActive = false;
		gestures.push_back({ GesturePhase::End, mCenter });
	}

	TouchGesture PointerPinchConverter::begin(Vec2F center) {
		mActive = true;
		mTotalScale = 1;
		return { GesturePhase::Begin, center };
	}

	std::optional<TouchGesture>
	PointerPinchConverter::update(Vec2F center, Vec2F pan, halnf totalScale, halnf rotationDegrees) {
		if (!mActive) return std::nullopt;

		TouchGesture gesture;
		gesture.phase = GesturePhase::Update;
		gesture.center = center;
		gesture.pan = pan;
		gesture.scale = mTotalScale > 0 && totalScale > 0 ? totalScale / mTotalScale : 1;
		gesture.rotation = wrapAngle(rotationDegrees * halnf(3.14159265358979) / 180);
		if (totalScale > 0) mTotalScale = totalScale;
		return gesture;
	}

	std::optional<TouchGesture> PointerPinchConverter::end(Vec2F center) {
		if (!mActive) return std::nullopt;
		mActive = false;
		return TouchGesture{ GesturePhase::End, center };
	}

	void accumulate(TouchGesture& into, const TouchGesture& next) {
		into.center = next.center;
		into.pan += next.pan;
		into.scale *= next.scale;
		into.rotation = wrapAngle(into.rotation + next.rotation);
	}
}
