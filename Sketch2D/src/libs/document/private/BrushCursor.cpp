#include "BrushCursor.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	const char* brushCursorStyleName(BrushCursorStyle style) {
		switch (style) {
			case BrushCursorStyle::Contour: return "Contour";
			case BrushCursorStyle::Brush: return "Brush";
			case BrushCursorStyle::Arrow: return "Arrow";
			case BrushCursorStyle::Cross: return "Cross";
		}
		return "Contour";
	}

	BrushCursorShape BrushCursorShape::build(
		const BrushCursorSettings& settings,
		const Brush& brush,
		Vec2F center,
		halnf zoom,
		halnf viewRotation,
		halnf pressure
	) {
		BrushCursorShape shape;
		shape.center = center;

		const bool special = brush.erases() || brush.smudges();
		const bool wantsContour =
			settings.style == BrushCursorStyle::Contour && (!special || settings.contourForEraserAndSmudge);
		if (!wantsContour) {
			shape.crosshair = settings.style == BrushCursorStyle::Cross;
			return shape;
		}

		const DabShape dab = brush.dabAt(pressure);
		const halnf radius = dab.radius() * zoom;
		const halnf roundness = std::clamp(dab.roundness, 0.01f, 1.0f);
		const halnf angle = dab.angleDegrees * 3.14159265f / 180.0f + viewRotation;
		const halnf cosine = std::cos(angle);
		const halnf sine = std::sin(angle);

		shape.contour = true;
		shape.radius = radius;
		shape.spotlight = settings.spotlight;
		shape.crosshair = settings.crosshairForSmallContour && radius * 2.0f < kSmallContourPixels;
		shape.outline.reserve((size_t) kContourSegments + 1);
		for (int4 index = 0; index <= kContourSegments; index++) {
			const halnf t = (halnf) index / (halnf) kContourSegments * 2.0f * 3.14159265f;
			const halnf x = std::cos(t) * radius;
			const halnf y = std::sin(t) * radius * roundness;
			shape.outline.push_back({ center.x + x * cosine - y * sine, center.y + x * sine + y * cosine });
		}
		return shape;
	}
}
