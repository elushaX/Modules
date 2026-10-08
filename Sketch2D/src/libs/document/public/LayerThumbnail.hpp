#pragma once

#include "Document.hpp"
#include "LayerLevels.hpp"

#include <vector>

namespace tp::sketch2d {

	class LayerThumbnail {
	public:
		static constexpr int4 kSize = 40;

		[[nodiscard]] static RectF documentBounds(const Document& document);
		[[nodiscard]] static std::vector<uint1> render(const Layer& layer, const RectF& worldBounds, int4 size = kSize);
		[[nodiscard]] static std::vector<uint1>
		renderLevel(const Layer& layer, const LayerLevels& levels, int4 level, const RectF& worldBounds, int4 size);
	};
}
