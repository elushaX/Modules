#pragma once

#include "Brush.hpp"
#include "Document.hpp"
#include "Vec.hpp"

#include <string>
#include <vector>

namespace tp::sketch2d {

	enum class BrushCursorStyle { Contour, Brush, Arrow, Cross };

	inline constexpr int4 kBrushCursorStyleCount = 4;

	[[nodiscard]] const char* brushCursorStyleName(BrushCursorStyle style);

	struct BrushCursorSettings {
		BrushCursorStyle style = BrushCursorStyle::Contour;
		bool contourForEraserAndSmudge = true;
		bool crosshairForSmallContour = true;
		bool spotlight = false;
		HueColor spotlightColor = { 60.0f, 1.0f, 1.0f };
		halnf spotlightAlpha = 0.4f;
		bool showForPen = true;
	};

	struct BrushCursorShape {
		static constexpr halnf kSmallContourPixels = 8.0f;
		static constexpr halnf kCrossArm = 6.0f;
		static constexpr int4 kContourSegments = 64;

		bool contour = false;
		bool crosshair = false;
		bool spotlight = false;
		Vec2F center = { 0, 0 };
		halnf radius = 0;
		std::vector<Vec2F> outline;

		static BrushCursorShape build(
			const BrushCursorSettings& settings,
			const Brush& brush,
			Vec2F center,
			halnf zoom,
			halnf viewRotation,
			halnf pressure = 1.0f
		);
	};
}
