#pragma once

#include "Graphics.hpp"
#include "LayerThumbnail.hpp"
#include "ThumbnailProvider.hpp"
#include "Map.hpp"

namespace tp::sketch2d {

	class CanvasThumbnails : public ThumbnailProvider {
	public:
		explicit CanvasThumbnails(Canvas& canvas);
		~CanvasThumbnails() override;

		CanvasThumbnails(const CanvasThumbnails&) = delete;
		CanvasThumbnails& operator=(const CanvasThumbnails&) = delete;

		[[nodiscard]] ui::ImageId thumbnail(const Document& document, const Layer& layer) override;

	private:
		Canvas& mCanvas;
		Map<ualni, Canvas::ImageHandle> mImages;
	};
}
