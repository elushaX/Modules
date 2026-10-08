#pragma once

#include "CanvasWidget.hpp"
#include "Graphics.hpp"
#include "TileRenderer.hpp"

namespace tp::sketch2d {

	class CanvasViewport : public ViewportRenderer {
	public:
		CanvasViewport(Canvas& canvas, TileRenderer& renderer);
		~CanvasViewport() override;

		CanvasViewport(const CanvasViewport&) = delete;
		CanvasViewport& operator=(const CanvasViewport&) = delete;

		void render(Document& document, const ViewSettings& settings, Vec2F viewportSize) override;
		[[nodiscard]] ui::ImageId image() const override { return { mImage.id }; }

		void setDebugOverlay(bool enabled) override { mRenderer.setDebug(enabled); }
		[[nodiscard]] bool debugOverlay() const override { return mRenderer.debug(); }
		[[nodiscard]] std::vector<LegendEntry> debugLegend() const override;
		[[nodiscard]] std::string statistics() const override;
		[[nodiscard]] bool isCatchingUp() const override { return mRenderer.hasPendingUploads(); }

	private:
		Canvas& mCanvas;
		TileRenderer& mRenderer;
		Document* mLastDocument = nullptr;
		Canvas::ImageHandle mImage;
	};
}
