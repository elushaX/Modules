#pragma once

#include "Document.hpp"

#include <vector>

namespace tp::sketch2d {

	struct ShadowBand {
		RectF rect;
		RGBA color;
	};

	constexpr halnf kCanvasShadowAlpha = 0.6f;
	constexpr int4 kCanvasShadowSteps = 6;
	constexpr halnf kCanvasShadowSpread = 18.0f;

	[[nodiscard]] RectF frameOnScreen(const Document& document);
	[[nodiscard]] RectF frameInView(const Document& document);
	[[nodiscard]] RectF visibleCanvasArea(const Document& document, Vec2F viewport);
	[[nodiscard]] std::vector<ShadowBand> canvasShadow(const RectF& frame);
}
