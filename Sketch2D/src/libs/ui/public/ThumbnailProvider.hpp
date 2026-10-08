#pragma once

#include "Document.hpp"
#include "Painter.hpp"

namespace tp::sketch2d {

	class ThumbnailProvider {
	public:
		virtual ~ThumbnailProvider() = default;
		[[nodiscard]] virtual ui::ImageId thumbnail(const Document& document, const Layer& layer) = 0;
	};
}
