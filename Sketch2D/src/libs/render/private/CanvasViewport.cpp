#include "CanvasViewport.hpp"

#include <algorithm>
#include <cstdio>

namespace tp::sketch2d {

	CanvasViewport::CanvasViewport(Canvas& canvas, TileRenderer& renderer) :
		mCanvas(canvas),
		mRenderer(renderer),
		mImage(canvas.createImageFromTextId(renderer.textureId(), { 0, 0 })) {}

	CanvasViewport::~CanvasViewport() { mCanvas.deleteImageHandle(mImage); }

	void CanvasViewport::render(Document& document, const ViewSettings& settings, Vec2F viewportSize) {
		mLastDocument = &document;
		mRenderer.setViewportSize(viewportSize);
		mRenderer.render(document, settings);
		mCanvas.updateTextureID(mImage, mRenderer.textureId());
	}

	std::string CanvasViewport::statistics() const {
		char text[160];
		std::snprintf(
			text,
			sizeof(text),
			"uploaded %llu pixels in %llu tiles  |  %llu tiles drawn at mip %d  |  %llu live",
			(unsigned long long) mRenderer.uploadedPixelsLastFrame(),
			(unsigned long long) mRenderer.uploadedTilesLastFrame(),
			(unsigned long long) mRenderer.drawnTilesLastFrame(),
			(int) mRenderer.mipLevelLastFrame(),
			(unsigned long long) (mLastDocument ? mLastDocument->tileCount() : 0)
		);
		std::string result = text;
		result += "  |  " + mRenderer.debugLegend();
		if (mLastDocument)
			result += "|  engine " + std::to_string(mLastDocument->engine().pending()) + "  paint " +
								std::to_string(mLastDocument->paintBacklog());
		return result;
	}

	std::vector<ViewportRenderer::LegendEntry> CanvasViewport::debugLegend() const {
		static const char* const kNames[] = { "done",   "uploading", "not on GPU", "preview",
																					"queued", "working",   "stale",      "empty" };
		std::vector<LegendEntry> entries;
		for (int4 index = 0; index < (int4) TileDebugState::Count; index++) {
			const TileDebugState state = (TileDebugState) index;
			RGBA color = TileRenderer::debugColor(state);
			color.a = std::min(1.0f, color.a * 2.5f);
			entries.push_back({ std::string(kNames[index]) + "  " + std::to_string(mRenderer.debugCount(state)), color });
		}
		return entries;
	}
}
