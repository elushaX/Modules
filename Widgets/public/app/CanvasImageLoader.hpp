#pragma once

#include "Graphics.hpp"
#include "Painter.hpp"

#include <map>
#include <string>

namespace tp::ui {

	class CanvasImageLoader : public ImageLoader {
	public:
		explicit CanvasImageLoader(Canvas& canvas);
		~CanvasImageLoader() override;

		CanvasImageLoader(const CanvasImageLoader&) = delete;
		CanvasImageLoader& operator=(const CanvasImageLoader&) = delete;

		[[nodiscard]] LoadedImage load(const std::string& path) override;

	private:
		Canvas& mCanvas;
		std::map<std::string, LoadedImage> mLoaded;
	};
}
