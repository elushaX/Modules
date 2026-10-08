#include "CanvasImageLoader.hpp"

using namespace tp;
using namespace tp::ui;

CanvasImageLoader::CanvasImageLoader(Canvas& canvas) :
	mCanvas(canvas) {}

CanvasImageLoader::~CanvasImageLoader() {
	for (const auto& [path, image] : mLoaded) {
		if (image.id.isValid()) mCanvas.deleteImageHandle({ image.id.value });
	}
}

LoadedImage CanvasImageLoader::load(const std::string& path) {
	const auto found = mLoaded.find(path);
	if (found != mLoaded.end()) return found->second;

	const Canvas::ImageHandle handle = mCanvas.createImageFromFile(path.c_str());
	LoadedImage image;
	if (handle.id) {
		image.id = { handle.id };
		image.size = mCanvas.imageSize(handle);
	}
	mLoaded.emplace(path, image);
	return image;
}
