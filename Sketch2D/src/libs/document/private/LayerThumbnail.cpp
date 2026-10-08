#include "LayerThumbnail.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	RectF LayerThumbnail::documentBounds(const Document& document) {
		bool any = false;
		int4 minX = 0;
		int4 minY = 0;
		int4 maxX = 0;
		int4 maxY = 0;

		document.forEachTile([&](const TileCoord& coord, Tile*) {
			if (!any) {
				minX = maxX = coord.x;
				minY = maxY = coord.y;
				any = true;
				return;
			}
			minX = std::min(minX, coord.x);
			minY = std::min(minY, coord.y);
			maxX = std::max(maxX, coord.x);
			maxY = std::max(maxY, coord.y);
		});

		if (!any) return { 0, 0, (halnf) kTileSize, (halnf) kTileSize };
		const halnf width = (halnf) ((maxX - minX + 1) * kTileSize);
		const halnf height = (halnf) ((maxY - minY + 1) * kTileSize);
		const halnf side = std::max(width, height);
		return {
			(halnf) (minX * kTileSize) - (side - width) / 2, (halnf) (minY * kTileSize) - (side - height) / 2, side, side
		};
	}

	std::vector<uint1> LayerThumbnail::render(const Layer& layer, const RectF& worldBounds, int4 size) {
		const halnf worldStep = worldBounds.size.x / (halnf) size;
		LayerLevels& levels = layer.levels();
		int4 level = 0;
		while (level + 1 < LayerLevels::kLevels && (halnf) (2 << level) <= worldStep)
			level++;
		levels.update(layer, level);
		if (level > 0 && levels.builtLevels() >= level) return renderLevel(layer, levels, level, worldBounds, size);

		std::vector<uint1> pixels((size_t) size * size * 4, 0);
		const halnf step = worldStep;
		const int4 samples = std::clamp((int4) std::ceil(step / 8.0f), 1, 8);

		for (int4 y = 0; y < size; y++) {
			for (int4 x = 0; x < size; x++) {
				halnf sum[4] = { 0, 0, 0, 0 };
				for (int4 sy = 0; sy < samples; sy++) {
					for (int4 sx = 0; sx < samples; sx++) {
						const halnf wx = worldBounds.pos.x + ((halnf) x + ((halnf) sx + 0.5f) / (halnf) samples) * step;
						const halnf wy = worldBounds.pos.y + ((halnf) y + ((halnf) sy + 0.5f) / (halnf) samples) * step;
						const int4 px = (int4) std::floor(wx);
						const int4 py = (int4) std::floor(wy);
						const TileCoord coord = { floordiv(px, kTileSize), floordiv(py, kTileSize) };
						const Tile* tile = layer.tileIfExists(coord);
						if (!tile) continue;

						const int4 lx = px - coord.x * kTileSize;
						const int4 ly = py - coord.y * kTileSize;
						const uint1* source = tile->pixels() + ((ualni) ly * kTileSize + (ualni) lx) * 4;
						for (int4 channel = 0; channel < 4; channel++)
							sum[channel] += source[channel];
					}
				}

				const halnf count = (halnf) (samples * samples);
				const halnf alpha = sum[3] / count;
				uint1* out = pixels.data() + ((size_t) y * size + (size_t) x) * 4;
				for (int4 channel = 0; channel < 3; channel++)
					out[channel] = alpha > 0 ? (uint1) std::lround(std::min(255.0f, sum[channel] / count * 255.0f / alpha)) : 0;
				out[3] = (uint1) std::lround(alpha);
			}
		}
		return pixels;
	}

	std::vector<uint1> LayerThumbnail::renderLevel(
		const Layer& layer, const LayerLevels& levels, int4 level, const RectF& worldBounds, int4 size
	) {
		(void) layer;
		std::vector<uint1> pixels((size_t) size * size * 4, 0);
		const halnf scale = (halnf) (1 << level);
		const halnf step = worldBounds.size.x / (halnf) size / scale;
		const int4 samples = std::clamp((int4) std::ceil(step), 1, 4);
		for (int4 y = 0; y < size; y++) {
			for (int4 x = 0; x < size; x++) {
				halnf sum[4] = { 0, 0, 0, 0 };
				for (int4 sy = 0; sy < samples; sy++) {
					for (int4 sx = 0; sx < samples; sx++) {
						const halnf wx = worldBounds.pos.x / scale + ((halnf) x + ((halnf) sx + 0.5f) / (halnf) samples) * step;
						const halnf wy = worldBounds.pos.y / scale + ((halnf) y + ((halnf) sy + 0.5f) / (halnf) samples) * step;
						const int4 px = (int4) std::floor(wx);
						const int4 py = (int4) std::floor(wy);
						const TileCoord coord = { floordiv(px, kTileSize), floordiv(py, kTileSize) };
						const Tile* tile = levels.tileAt(level, coord);
						if (!tile) continue;
						const uint1* source =
							tile->pixels() +
							((ualni) (py - coord.y * kTileSize) * kTileSize + (ualni) (px - coord.x * kTileSize)) * 4;
						for (int4 channel = 0; channel < 4; channel++)
							sum[channel] += source[channel];
					}
				}
				const halnf count = (halnf) (samples * samples);
				const halnf alpha = sum[3] / count;
				uint1* out = pixels.data() + ((size_t) y * size + (size_t) x) * 4;
				for (int4 channel = 0; channel < 3; channel++)
					out[channel] = alpha > 0 ? (uint1) std::lround(std::min(255.0f, sum[channel] / count * 255.0f / alpha)) : 0;
				out[3] = (uint1) std::lround(alpha);
			}
		}
		return pixels;
	}
}
