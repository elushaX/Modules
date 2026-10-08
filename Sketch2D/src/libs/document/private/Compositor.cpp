#include "Compositor.hpp"
#include "Profiler.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace tp::sketch2d {

	namespace {
		halnf clamp01(halnf value) { return value < 0 ? 0 : (value > 1 ? 1 : value); }

		halnf unpremultiplied(halnf channel, halnf alpha) { return alpha > 0 ? clamp01(channel / alpha) : 0; }

		halnf colorDodge(halnf below, halnf above) { return above >= 1 ? 1 : std::min(1.0f, below / (1 - above)); }

		halnf colorBurn(halnf below, halnf above) { return above <= 0 ? 0 : std::max(0.0f, 1 + (below - 1) / above); }

		struct Premultiplied {
			halnf c[3];
			halnf a;
		};

		Premultiplied fromPixel(const BlendPixel& pixel, halnf scale) {
			return { { pixel.r * scale, pixel.g * scale, pixel.b * scale }, pixel.a * scale };
		}

		halnf luma(const halnf c[3]) { return 0.2990f * c[0] + 0.5870f * c[1] + 0.1140f * c[2]; }

		void colorFromLumaChroma(halnf result[3], halnf y, halnf cb, halnf cr, halnf alpha) {
			const halnf dr = 1.402f * cr;
			const halnf dg = -0.344136f * cb - 0.714136f * cr;
			const halnf db = 1.772f * cb;

			halnf scale = 1;
			for (const halnf delta : { dr, dg, db }) {
				if (y + delta < 0 && delta != 0) scale = std::min(scale, -y / delta);
				if (y + delta > alpha && delta != 0) scale = std::min(scale, (alpha - y) / delta);
			}
			scale = std::max(0.0f, scale);

			result[0] = y + dr * scale;
			result[1] = y + dg * scale;
			result[2] = y + db * scale;
		}

		halnf blendedChannel(BlendMode mode, halnf d, halnf da, halnf s, halnf sa, halnf opacity) {
			switch (mode) {
				case BlendMode::Multiply: return s * d;
				case BlendMode::Screen: return sa * da - (sa - s) * (da - d);
				case BlendMode::Overlay: return d > da / 2 ? sa * da - 2 * (sa - s) * (da - d) : 2 * s * d;
				case BlendMode::Lighten: return std::max(unpremultiplied(d, da), unpremultiplied(s, sa)) * sa * da;
				case BlendMode::Darken: return std::min(unpremultiplied(d, da), unpremultiplied(s, sa)) * sa * da;
				case BlendMode::ColorDodge: return colorDodge(unpremultiplied(d, da), unpremultiplied(s, sa)) * sa * da;
				case BlendMode::ColorBurn: return colorBurn(unpremultiplied(d, da), unpremultiplied(s, sa)) * sa * da;
				case BlendMode::SoftDodge: return std::min((s + 1) * d, da) * sa;
				case BlendMode::SoftBurn: return std::min(((s * opacity - sa + 1) * 0.5f + 0.5f) * d, da) * sa;
				default: return s;
			}
		}

		BlendPixel pixelAt(const Tile* tile, ualni index) {
			if (!tile) return {};
			const uint1* px = tile->pixels() + index * 4;
			return { px[0] / 255.0f, px[1] / 255.0f, px[2] / 255.0f, px[3] / 255.0f };
		}

		void store(uint1* px, const BlendPixel& pixel) {
			px[0] = (uint1) std::lround(clamp01(pixel.r) * 255.0f);
			px[1] = (uint1) std::lround(clamp01(pixel.g) * 255.0f);
			px[2] = (uint1) std::lround(clamp01(pixel.b) * 255.0f);
			px[3] = (uint1) std::lround(clamp01(pixel.a) * 255.0f);
		}

		ualni mix(ualni hash, ualni value) {
			hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
			return hash;
		}
	}

	BlendPixel blendPixel(BlendMode mode, const BlendPixel& below, const BlendPixel& above, halnf opacity) {
		const Premultiplied d = fromPixel(below, 1);
		const Premultiplied s = fromPixel(above, opacity);
		if (s.a <= 0) return below;

		halnf result[3];
		if (mode == BlendMode::Normal) {
			for (int4 channel = 0; channel < 3; channel++)
				result[channel] = s.c[channel] + d.c[channel] * (1 - s.a);
			return { result[0], result[1], result[2], s.a + d.a * (1 - s.a) };
		}

		if (mode == BlendMode::Color) {
			halnf over[3];
			for (int4 channel = 0; channel < 3; channel++)
				over[channel] = s.c[channel] + d.c[channel] * (1 - s.a);
			const halnf overAlpha = s.a + d.a * (1 - s.a);
			const halnf cb = (0.5f * over[2] - 0.331264f * over[1] - 0.168736f * over[0]) / overAlpha;
			const halnf cr = (0.5f * over[0] - 0.418688f * over[1] - 0.081312f * over[2]) / overAlpha;
			colorFromLumaChroma(result, luma(d.c), cb * d.a, cr * d.a, d.a);
			for (halnf& channel : result)
				channel *= s.a;
		} else {
			for (int4 channel = 0; channel < 3; channel++)
				result[channel] = blendedChannel(mode, d.c[channel], d.a, s.c[channel], s.a, opacity);
		}

		for (int4 channel = 0; channel < 3; channel++)
			result[channel] += d.c[channel] * (1 - s.a) + s.c[channel] * (1 - d.a);
		return { result[0], result[1], result[2], s.a + d.a * (1 - s.a) };
	}

	ualni Compositor::signatureOf(const std::vector<StackEntry>& stack) {
		ualni hash = 1469598103934665603ull;
		for (const StackEntry& entry : stack) {
			const Layer::Properties& properties = entry.layer->properties();
			hash = mix(hash, entry.layer->id());
			hash = mix(hash, (ualni) (uintptr_t) entry.tile);
			hash = mix(hash, (ualni) std::lround(properties.opacity * 65535.0f));
			hash = mix(hash, (ualni) properties.blend);
			hash = mix(hash, properties.clipped ? 1 : 0);
		}
		return hash;
	}

	void Compositor::PixelRect::include(const PixelRect& other) {
		if (other.isEmpty()) return;
		if (isEmpty()) {
			*this = other;
			return;
		}
		x0 = std::min(x0, other.x0);
		y0 = std::min(y0, other.y0);
		x1 = std::max(x1, other.x1);
		y1 = std::max(y1, other.y1);
	}

	Compositor::PixelRect Compositor::PixelRect::whole() { return { 0, 0, kTileSize, kTileSize }; }

	DirtyBands Compositor::pendingChanges(const std::vector<StackEntry>& stack) {
		DirtyBands changed;
		for (const StackEntry& entry : stack) {
			if (entry.tile) changed.include(entry.tile->compositeBands());
		}
		return changed;
	}

	bool Compositor::isPlainCopy(const std::vector<StackEntry>& stack) {
		if (stack.size() != 1 || !stack.front().tile) return false;
		const Layer& layer = *stack.front().layer;
		return layer.blend() == BlendMode::Normal && !layer.clipped() && layer.opacity() >= 1.0f;
	}

	bool Compositor::isNormalStack(const std::vector<StackEntry>& stack) {
		for (const StackEntry& entry : stack) {
			if (entry.layer->blend() != BlendMode::Normal || entry.layer->clipped()) return false;
		}
		return true;
	}

	void Compositor::compositeRows(const std::vector<StackEntry>& stack, uint1* out, int4 x0, int4 x1, int4 y) {
		const ualni rowStart = (ualni) y * kTileSize;
		if (isPlainCopy(stack)) {
			std::memcpy(
				out + (rowStart + (ualni) x0) * 4,
				stack.front().tile->pixels() + (rowStart + (ualni) x0) * 4,
				(size_t) (x1 - x0) * 4
			);
			return;
		}

		if (isNormalStack(stack)) {
			uint4 opacity[64];
			const size_t layers = std::min(stack.size(), (size_t) 64);
			for (size_t index = 0; index < layers; index++)
				opacity[index] = (uint4) std::lround(std::clamp(stack[index].layer->opacity(), 0.0f, 1.0f) * 255.0f);

			for (int4 x = x0; x < x1; x++) {
				const ualni index = rowStart + (ualni) x;
				uint4 result[4] = { 0, 0, 0, 0 };
				for (size_t layer = 0; layer < layers; layer++) {
					const Tile* tile = stack[layer].tile;
					if (!tile) continue;
					const uint1* above = tile->pixels() + index * 4;
					if (above[3] == 0 || opacity[layer] == 0) continue;
					uint4 source[4];
					for (int4 channel = 0; channel < 4; channel++)
						source[channel] = (above[channel] * opacity[layer] + 127) / 255;
					const uint4 keep = 255 - source[3];
					for (int4 channel = 0; channel < 4; channel++)
						result[channel] = source[channel] + (result[channel] * keep + 127) / 255;
				}
				uint1* target = out + index * 4;
				for (int4 channel = 0; channel < 4; channel++)
					target[channel] = (uint1) std::min(result[channel], 255u);
			}
			return;
		}

		for (int4 x = x0; x < x1; x++) {
			const ualni index = rowStart + (ualni) x;
			BlendPixel result;
			halnf clipBase = 0;

			for (const StackEntry& entry : stack) {
				const Layer& layer = *entry.layer;
				BlendPixel above = pixelAt(entry.tile, index);

				if (layer.clipped()) {
					above.r *= clipBase;
					above.g *= clipBase;
					above.b *= clipBase;
					above.a *= clipBase;
				} else {
					clipBase = above.a;
				}

				if (above.a <= 0) continue;
				result = blendPixel(layer.blend(), result, above, layer.opacity());
			}

			store(out + index * 4, result);
		}
	}

	void Compositor::composite(const std::vector<StackEntry>& stack, Tile& target, const PixelRect& rect) {
		for (int4 y = rect.y0; y < rect.y1; y++)
			compositeRows(stack, target.pixels(), rect.x0, rect.x1, y);
		target.markDirty({ (halnf) rect.x0, (halnf) rect.y0, (halnf) (rect.x1 - rect.x0), (halnf) (rect.y1 - rect.y0) });
	}

	void Compositor::composite(const std::vector<StackEntry>& stack, Tile& target, const DirtyBands& bands) {
		for (int4 band = 0; band < DirtyBands::kBands; band++) {
			const DirtySpan& span = bands.band(band);
			if (span.isEmpty()) continue;
			for (int4 y = span.y0; y < span.y1; y++)
				compositeRows(stack, target.pixels(), span.x0, span.x1, y);
			target.markDirty({ (halnf) span.x0, (halnf) span.y0, (halnf) (span.x1 - span.x0), (halnf) (span.y1 - span.y0) });
		}
	}

	bool Compositor::isVisibleIn(const Document&, const Layer& layer, bool& baseVisible) {
		if (!layer.clipped()) baseVisible = layer.visible();
		return layer.visible() && (!layer.clipped() || baseVisible);
	}

	std::vector<Compositor::StackEntry> Compositor::stackAt(const Document& document, const TileCoord& coord) {
		std::vector<StackEntry> stack;
		bool baseVisible = false;

		for (const auto& layer : document.layers()) {
			if (!isVisibleIn(document, *layer, baseVisible)) continue;
			Tile* preview = layer->levels().basePreview(coord);
			stack.push_back({ layer.get(), preview ? preview : layer->tileIfExists(coord) });
		}
		return stack;
	}

	void Compositor::flatten(const std::vector<const Layer*>& layers, const TileCoord& coord, Tile& target) {
		std::vector<StackEntry> stack;
		for (const Layer* layer : layers)
			stack.push_back({ layer, layer->tileIfExists(coord) });
		composite(stack, target, PixelRect::whole());
	}

	LayerLevels* Compositor::levelsOf(const Layer& layer) const { return &layer.levels(); }

	void Compositor::syncLevels(const Document& document, int4 level) {
		const Engine& engine = document.engine();
		for (const auto& layer : document.layers()) {
			const Layer* owner = layer.get();
			layer->levels().setPendingCheck([&engine, owner](const TileCoord& coord) {
				return engine.hasWork(*owner, coord);
			});
			layer->levels().update(*layer, level, mParallel);
		}
	}

	void Compositor::update(const Document& document) { update(document, 0); }

	void Compositor::update(const Document& document, int4 level) {
		{
			TP_PROFILE_SCOPE("canvas.mips");
			syncLevels(document, level);
		}
		const bool levelChanged = false;
		mLevel = level;
		TP_PROFILE_SCOPE("canvas.stack");
		if (level == 0) {
			updateBase(document, levelChanged);
			return;
		}
		updateLevel(document, level, levelChanged);
	}

	void Compositor::updateLevel(const Document& document, int4 level, bool levelChanged) {
		LevelCache& cache = mLevels[level];
		mFrame++;
		mLastComposited = 0;
		mLastPixels = 0;
		if (!levelChanged) mRemoved.clear();
		mChanged.clear();

		ualni layout = 1469598103934665603ull;
		for (const auto& layer : document.layers()) {
			const Layer::Properties& properties = layer->properties();
			layout = mix(layout, layer->id());
			layout = mix(layout, layer->serial());
			layout = mix(layout, properties.visible ? 1 : 0);
			layout = mix(layout, (ualni) std::lround(properties.opacity * 65535.0f));
			layout = mix(layout, (ualni) properties.blend);
			layout = mix(layout, properties.clipped ? 1 : 0);
		}
		const bool full = !cache.hasLayout || layout != cache.layout;
		cache.layout = layout;
		cache.hasLayout = true;

		Map<ualni, TileCoord> coords;
		for (const auto& layer : document.layers()) {
			LayerLevels* levels = levelsOf(*layer);
			if (!levels) continue;
			if (full) {
				if (layer->visible())
					levels->forEachTile(level, [&coords](const TileCoord& coord, Tile*) { coords.put(coord.key(), coord); });
				levels->takeChanges(level, [](const TileCoord&) {});
			} else {
				levels->takeChanges(level, [&coords, &layer](const TileCoord& coord) {
					if (layer->visible()) coords.put(coord.key(), coord);
				});
			}
		}
		if (full)
			for (auto it : cache.tiles)
				coords.put(it->key, it->val.coord);

		struct Job {
			std::vector<StackEntry> stack;
			Tile* target;
		};
		std::vector<Job> jobs;
		for (auto it : coords) {
			const TileCoord coord = it->val;
			std::vector<StackEntry> stack;
			bool baseVisible = false;
			bool any = false;
			for (const auto& layer : document.layers()) {
				if (!isVisibleIn(document, *layer, baseVisible)) continue;
				LayerLevels* levels = levelsOf(*layer);
				Tile* tile = levels ? levels->tileAt(level, coord) : nullptr;
				any = any || tile;
				stack.push_back({ layer.get(), tile });
			}
			auto found = cache.tiles.presents(coord.key());
			if (!any) {
				if (found.isValid()) {
					mRemoved.push_back(cache.tiles.getSlotVal(found).tile);
					cache.tiles.remove(coord.key());
					mChanged.push_back(coord);
				}
				continue;
			}
			if (!found.isValid()) cache.tiles.put(coord.key(), { coord, std::make_shared<Tile>(), 0, mFrame });
			CompositeTile& entry = cache.tiles.get(coord.key());
			entry.frame = mFrame;
			entry.signature = signatureOf(stack);
			jobs.push_back({ std::move(stack), entry.tile.get() });
			mChanged.push_back(coord);
			mLastComposited++;
			mLastPixels += (ualni) kTileSize * kTileSize;
		}

		const auto run = [&jobs](size_t index) {
			Job& job = jobs[index];
			for (int4 y = 0; y < kTileSize; y++)
				compositeRows(job.stack, job.target->pixels(), 0, kTileSize, y);
		};
		if (mParallel) mParallel->run(jobs.size(), run);
		else
			for (size_t index = 0; index < jobs.size(); index++)
				run(index);
		for (Job& job : jobs)
			job.target->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
	}

	void Compositor::updateBase(const Document& document, bool levelChanged) {
		LevelCache& cache = mLevels[0];
		mFrame++;
		mLastComposited = 0;
		mLastPixels = 0;

		ualni layout = 1469598103934665603ull;
		for (const auto& layer : document.layers()) {
			const Layer::Properties& properties = layer->properties();
			layout = mix(layout, layer->id());
			layout = mix(layout, layer->serial());
			layout = mix(layout, properties.visible ? 1 : 0);
			layout = mix(layout, (ualni) std::lround(properties.opacity * 65535.0f));
			layout = mix(layout, (ualni) properties.blend);
			layout = mix(layout, properties.clipped ? 1 : 0);
		}
		const bool full = !cache.hasLayout || layout != cache.layout;
		cache.layout = layout;
		cache.hasLayout = true;

		Map<ualni, TileCoord> coords;
		if (full) {
			document.forEachVisibleTile([&](const TileCoord& coord, Tile*) { coords.put(coord.key(), coord); });
			for (auto it : cache.tiles)
				coords.put(it->key, it->val.coord);
		}
		for (const auto& layer : document.layers()) {
			if (!full && layer->visible())
				layer->forEachChanged([&](const TileCoord& coord) { coords.put(coord.key(), coord); });
			layer->clearChanges();
		}
		mChanged.clear();
		if (!levelChanged) mRemoved.clear();

		struct Job {
			std::vector<StackEntry> stack;
			Tile* target;
			DirtyBands bands;
			bool whole;
		};
		std::vector<Job> jobs;

		for (auto it : coords) {
			const TileCoord coord = it->val;
			std::vector<StackEntry> stack = stackAt(document, coord);
			if (std::none_of(stack.begin(), stack.end(), [](const StackEntry& entry) { return entry.tile; })) {
				auto gone = cache.tiles.presents(coord.key());
				if (gone.isValid()) {
					mRemoved.push_back(cache.tiles.getSlotVal(gone).tile);
					cache.tiles.remove(coord.key());
					mChanged.push_back(coord);
				}
				continue;
			}

			const ualni signature = signatureOf(stack);
			const DirtyBands pending = pendingChanges(stack);
			for (const StackEntry& entry : stack) {
				if (!entry.tile) continue;
				entry.tile->clearDirty();
				entry.tile->clearCompositeBands();
			}

			auto found = cache.tiles.presents(coord.key());
			if (found.isValid()) {
				CompositeTile& existing = cache.tiles.getSlotVal(found);
				existing.frame = mFrame;
				const bool same = existing.signature == signature;
				existing.signature = signature;
				if (same && pending.isEmpty()) continue;
				mLastPixels += same ? pending.area() : (ualni) kTileSize * kTileSize;
				jobs.push_back({ std::move(stack), existing.tile.get(), pending, !same });
				mChanged.push_back(coord);
				mLastComposited++;
				continue;
			}

			CompositeTile created{ coord, std::make_shared<Tile>(), signature, mFrame };
			cache.tiles.put(coord.key(), created);
			mChanged.push_back(coord);
			jobs.push_back({ std::move(stack), created.tile.get(), {}, true });
			mLastComposited++;
			mLastPixels += (ualni) kTileSize * kTileSize;
		}

		const auto run = [&jobs](size_t index) {
			Job& job = jobs[index];
			if (job.whole) {
				for (int4 y = 0; y < kTileSize; y++)
					compositeRows(job.stack, job.target->pixels(), 0, kTileSize, y);
				return;
			}
			for (int4 band = 0; band < DirtyBands::kBands; band++) {
				const DirtySpan& span = job.bands.band(band);
				for (int4 y = span.y0; y < span.y1; y++)
					compositeRows(job.stack, job.target->pixels(), span.x0, span.x1, y);
			}
		};
		if (mParallel) mParallel->run(jobs.size(), run);
		else
			for (size_t index = 0; index < jobs.size(); index++)
				run(index);

		for (Job& job : jobs) {
			if (job.whole) {
				job.target->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
				continue;
			}
			for (int4 band = 0; band < DirtyBands::kBands; band++) {
				const DirtySpan& span = job.bands.band(band);
				if (span.isEmpty()) continue;
				job.target->markDirty(
					{ (halnf) span.x0, (halnf) span.y0, (halnf) (span.x1 - span.x0), (halnf) (span.y1 - span.y0) }
				);
			}
		}
	}

	Tile* Compositor::tileAt(int4 level, const TileCoord& coord) const {
		auto found = mLevels[level].tiles.presents(coord.key());
		return found.isValid() ? mLevels[level].tiles.getSlotVal(found).tile.get() : nullptr;
	}
}
