#pragma once

#include "Graphics.hpp"
#include "LibraryView.hpp"

#include <map>

namespace tp::sketch2d {

	class CanvasPreviewImages : public PreviewImages {
	public:
		explicit CanvasPreviewImages(Canvas& canvas);
		~CanvasPreviewImages() override;

		CanvasPreviewImages(const CanvasPreviewImages&) = delete;
		CanvasPreviewImages& operator=(const CanvasPreviewImages&) = delete;

		[[nodiscard]] ui::ImageId image(const std::string& key, const SketchPreview& preview) override;
		[[nodiscard]] ui::ImageId cached(const std::string& key) const override;
		void forget(const std::string& key) override;

	private:
		Canvas& mCanvas;
		std::map<std::string, Canvas::ImageHandle> mImages;
	};
}
