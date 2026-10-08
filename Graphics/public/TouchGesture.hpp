#pragma once

#include "Vec.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tp {

	enum class GesturePhase { Begin, Update, End };

	struct TouchGesture {
		GesturePhase phase = GesturePhase::Update;
		Vec2F center = { 0, 0 };
		Vec2F pan = { 0, 0 };
		halnf scale = 1;
		halnf rotation = 0;
		int32_t tapFingers = 0;

		[[nodiscard]] bool operator==(const TouchGesture& other) const = default;
	};

	struct TouchPoint {
		int32_t id = 0;
		Vec2F position = { 0, 0 };
	};

	class TouchGestureTracker {
	public:
		static constexpr uint32_t kTapMaximumMs = 300;
		static constexpr halnf kTapMaximumTravel = 20;

		void update(const std::vector<TouchPoint>& touches, std::vector<TouchGesture>& gestures, uint32_t timeMs = 0);
		void cancel(std::vector<TouchGesture>& gestures);

		[[nodiscard]] bool isActive() const { return mActive; }

	private:
		void track(const TouchPoint& first, const TouchPoint& second);
		void followTap(const std::vector<TouchPoint>& touches, uint32_t timeMs);
		[[nodiscard]] int32_t finishedTapFingers(uint32_t timeMs) const;

		std::vector<TouchPoint> mTapStarts;
		uint32_t mTapStartMs = 0;
		bool mTapStillPossible = false;

		bool mActive = false;
		int32_t mFirst = 0;
		int32_t mSecond = 0;
		Vec2F mCenter = { 0, 0 };
		halnf mDistance = 0;
		halnf mAngle = 0;
	};

	void accumulate(TouchGesture& into, const TouchGesture& next);

	// Converts touchpad pinch updates (scale relative to the gesture start, pan and rotation per event) into
	// incremental TouchGesture updates.
	class PointerPinchConverter {
	public:
		[[nodiscard]] TouchGesture begin(Vec2F center);
		[[nodiscard]] std::optional<TouchGesture> update(Vec2F center, Vec2F pan, halnf totalScale, halnf rotationDegrees);
		[[nodiscard]] std::optional<TouchGesture> end(Vec2F center);

	private:
		halnf mTotalScale = 1;
		bool mActive = false;
	};
}
