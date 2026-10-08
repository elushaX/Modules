#include "CanvasThumbnails.hpp"
#include "Profiler.hpp"

namespace tp::sketch2d {

	CanvasThumbnails::CanvasThumbnails(Canvas& canvas) :
		mCanvas(canvas) {}

	CanvasThumbnails::~CanvasThumbnails() {
		for (auto entry : mImages)
			mCanvas.deleteImageHandle(entry->val);
	}

	ui::ImageId CanvasThumbnails::thumbnail(const Document& document, const Layer& layer) {
		TP_PROFILE_SCOPE("layers.thumbnail");
		std::vector<uint1> pixels;
		document.whilePaintPaused([&]() {
			pixels = LayerThumbnail::render(layer, LayerThumbnail::documentBounds(document));
		});

		auto slot = mImages.presents(layer.id());
		if (slot.isValid()) {
			const Canvas::ImageHandle image = mImages.getSlotVal(slot);
			mCanvas.updateImageRGBA(image, pixels.data());
			return { image.id };
		}

		const Canvas::ImageHandle image =
			mCanvas.createImageFromMemoryRGBA(LayerThumbnail::kSize, LayerThumbnail::kSize, pixels.data());
		mImages.put(layer.id(), image);
		return { image.id };
	}
}
