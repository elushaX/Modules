#include "CanvasFrameGeometry.hpp"

#include <algorithm>

namespace tp::sketch2d {

	RectF frameOnScreen(const Document& document) {
		const RectF world = document.frame().rect();
		const Vec2F topLeft = document.worldToScreen(world.pos);
		const Vec2F bottomRight = document.worldToScreen(world.pos + world.size);
		return { topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y };
	}

	RectF frameInView(const Document& document) {
		const RectF world = document.frame().rect();
		const Vec2F topLeft = document.worldToView(world.pos);
		const Vec2F bottomRight = document.worldToView(world.pos + world.size);
		return { topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y };
	}

	RectF visibleCanvasArea(const Document& document, Vec2F viewport) {
		const RectF bounds = document.viewBoundsOfScreen(viewport);
		if (document.isInfinite()) return bounds;

		const RectF frame = frameInView(document);
		const halnf x0 = std::clamp(frame.pos.x, bounds.pos.x, bounds.pos.x + bounds.size.x);
		const halnf y0 = std::clamp(frame.pos.y, bounds.pos.y, bounds.pos.y + bounds.size.y);
		const halnf x1 = std::clamp(frame.pos.x + frame.size.x, bounds.pos.x, bounds.pos.x + bounds.size.x);
		const halnf y1 = std::clamp(frame.pos.y + frame.size.y, bounds.pos.y, bounds.pos.y + bounds.size.y);
		return { x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0) };
	}

	std::vector<ShadowBand> canvasShadow(const RectF& frame) {
		std::vector<ShadowBand> bands;
		const halnf step = kCanvasShadowSpread / (halnf) kCanvasShadowSteps;

		for (int4 index = kCanvasShadowSteps; index >= 1; index--) {
			const halnf grow = step * (halnf) index;
			const halnf falloff = 1.0f - (halnf) (index - 1) / (halnf) kCanvasShadowSteps;
			const halnf alpha = kCanvasShadowAlpha * falloff * falloff / (halnf) kCanvasShadowSteps;
			bands.push_back(
				{ { frame.pos.x - grow, frame.pos.y - grow + grow / 3, frame.size.x + grow * 2, frame.size.y + grow * 2 },
					{ 0, 0, 0, alpha } }
			);
		}
		return bands;
	}
}
