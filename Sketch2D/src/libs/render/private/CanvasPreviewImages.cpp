#include "CanvasPreviewImages.hpp"

namespace tp::sketch2d {

	CanvasPreviewImages::CanvasPreviewImages(Canvas& canvas) :
		mCanvas(canvas) {}

	CanvasPreviewImages::~CanvasPreviewImages() {
		for (auto& entry : mImages)
			mCanvas.deleteImageHandle(entry.second);
	}

	ui::ImageId CanvasPreviewImages::image(const std::string& key, const SketchPreview& preview) {
		auto found = mImages.find(key);
		if (found != mImages.end()) return { found->second.id };

		const Canvas::ImageHandle handle =
			mCanvas.createImageFromMemoryRGBA(preview.size, preview.size, preview.rgba.data());
		mImages[key] = handle;
		return { handle.id };
	}

	ui::ImageId CanvasPreviewImages::cached(const std::string& key) const {
		auto found = mImages.find(key);
		return found == mImages.end() ? ui::ImageId{} : ui::ImageId{ found->second.id };
	}

	void CanvasPreviewImages::forget(const std::string& key) {
		auto found = mImages.find(key);
		if (found == mImages.end()) return;
		mCanvas.deleteImageHandle(found->second);
		mImages.erase(found);
	}
}
