#pragma once

#include "InputRecording.hpp"
#include "Rect.hpp"

#include <cstdint>

namespace tp::sketch2d::bench {

	struct ScribbleOptions {
		RectF area = { 0, 0, 800, 600 };
		double durationMs = 2500;
		double sampleIntervalMs = 4;
		double speedPixelsPerSecond = 2600;
		double strokeMs = 700;
		double gapMs = 60;
		bool pen = true;
		uint32_t seed = 1;
	};

	[[nodiscard]] InputRecording fastScribble(const ScribbleOptions& options);
}
