#include "Document.hpp"

#include <atomic>
#include "DabMask.hpp"
#include "Profiler.hpp"

#include "Compositor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>

namespace tp::sketch2d {

	namespace {
		template <typename Visit>
		void forEachTileSpan(int4 originX, int4 originY, int4 diameter, const Visit& visit) {
			const int4 endX = originX + diameter;
			const int4 endY = originY + diameter;
			for (int4 ty = floordiv(originY, kTileSize); ty <= floordiv(endY - 1, kTileSize); ty++) {
				for (int4 tx = floordiv(originX, kTileSize); tx <= floordiv(endX - 1, kTileSize); tx++) {
					const int4 x0 = std::max(originX, tx * kTileSize);
					const int4 y0 = std::max(originY, ty * kTileSize);
					const int4 x1 = std::min(endX, (tx + 1) * kTileSize);
					const int4 y1 = std::min(endY, (ty + 1) * kTileSize);
					visit(TileCoord{ tx, ty }, x0, y0, x1, y1);
				}
			}
		}

		void decodeMixPixel(const uint1* stored, halnf* mix, bool linear) {
			const halnf alpha = stored[3] / 255.0f;
			mix[3] = alpha;
			for (int4 channel = 0; channel < 3; channel++) {
				const halnf premultiplied = stored[channel] / 255.0f;
				if (!linear || alpha <= 0.0f) {
					mix[channel] = premultiplied;
					continue;
				}
				mix[channel] = gammaToLinear(std::min(premultiplied / alpha, 1.0f)) * alpha;
			}
		}

		void encodeMixPixel(const halnf* mix, uint1* stored, bool linear) {
			const halnf alpha = std::clamp(mix[3], 0.0f, 1.0f);
			stored[3] = (uint1) std::lround(alpha * 255.0f);
			for (int4 channel = 0; channel < 3; channel++) {
				halnf premultiplied = std::clamp(mix[channel], 0.0f, alpha);
				if (linear && alpha > 0.0f) premultiplied = linearToGamma(premultiplied / alpha) * alpha;
				stored[channel] = (uint1) std::lround(premultiplied * 255.0f);
			}
		}

		void keepSmudgedAlpha(halnf* mix, halnf alpha) {
			if (mix[3] <= 0.0f) {
				mix[0] = mix[1] = mix[2] = 0.0f;
				mix[3] = alpha;
				return;
			}
			const halnf scale = alpha / mix[3];
			for (int4 channel = 0; channel < 3; channel++)
				mix[channel] *= scale;
			mix[3] = alpha;
		}

		int4 smudgeBlurRadius(halnf blur) { return (int4) std::lround(blur); }

		template <typename Pixel>
		void boxBlurLine(int4 size, int4 radius, const Pixel& pixel) {
			std::vector<halnf> line((size_t) size * 4);
			for (int4 index = 0; index < size; index++)
				std::copy_n(pixel(index), 4, line.data() + (size_t) index * 4);

			halnf sum[4] = {};
			for (int4 offset = -radius; offset <= radius; offset++) {
				const halnf* source = line.data() + (size_t) std::clamp(offset, 0, size - 1) * 4;
				for (int4 channel = 0; channel < 4; channel++)
					sum[channel] += source[channel];
			}

			const halnf window = (halnf) (2 * radius + 1);
			for (int4 index = 0; index < size; index++) {
				halnf* target = pixel(index);
				for (int4 channel = 0; channel < 4; channel++)
					target[channel] = sum[channel] / window;

				const halnf* leaving = line.data() + (size_t) std::clamp(index - radius, 0, size - 1) * 4;
				const halnf* entering = line.data() + (size_t) std::clamp(index + radius + 1, 0, size - 1) * 4;
				for (int4 channel = 0; channel < 4; channel++)
					sum[channel] += entering[channel] - leaving[channel];
			}
		}

		SmudgePatch blurredSmudgePatch(const SmudgePatch& patch, int4 radius) {
			if (radius <= 0) return patch;

			SmudgePatch result = patch;
			const int4 size = patch.diameter;
			for (int4 y = 0; y < size; y++)
				boxBlurLine(size, radius, [&](int4 x) { return result.at(x, y); });
			for (int4 x = 0; x < size; x++)
				boxBlurLine(size, radius, [&](int4 y) { return result.at(x, y); });
			return result;
		}

		void resampleSmudgeCarry(SmudgePatch& carry, int4 diameter) {
			SmudgePatch resized;
			resized.diameter = diameter;
			resized.rgba.assign((size_t) diameter * diameter * 4, 0.0f);
			for (int4 y = 0; y < diameter; y++) {
				for (int4 x = 0; x < diameter; x++) {
					const int4 sx = std::min(x * carry.diameter / diameter, carry.diameter - 1);
					const int4 sy = std::min(y * carry.diameter / diameter, carry.diameter - 1);
					std::copy_n(carry.at(sx, sy), 4, resized.at(x, y));
				}
			}
			carry = std::move(resized);
		}

		void resaturateSmudgeCarry(SmudgePatch& carry, const Brush& brush, halnf resaturation) {
			if (resaturation <= 0.0f) return;
			const halnf color[3] = { brush.color.r, brush.color.g, brush.color.b };
			for (int4 y = 0; y < carry.diameter; y++) {
				for (int4 x = 0; x < carry.diameter; x++) {
					halnf* pixel = carry.at(x, y);
					for (int4 channel = 0; channel < 3; channel++)
						pixel[channel] += (color[channel] - pixel[channel]) * resaturation;
					pixel[3] += (1.0f - pixel[3]) * resaturation;
				}
			}
		}

		SmudgePatch readSmudgePatch(const Layer& layer, int4 originX, int4 originY, int4 diameter, bool linear) {
			SmudgePatch patch;
			patch.diameter = diameter;
			patch.rgba.assign((size_t) diameter * diameter * 4, 0.0f);

			forEachTileSpan(originX, originY, diameter, [&](TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1) {
				const Tile* tile = layer.tileIfExists(coord);
				if (!tile) return;
				for (int4 y = y0; y < y1; y++) {
					for (int4 x = x0; x < x1; x++) {
						const int4 lx = x - coord.x * kTileSize;
						const int4 ly = y - coord.y * kTileSize;
						const uint1* source = tile->pixels() + ((ualni) ly * kTileSize + lx) * 4;
						decodeMixPixel(source, patch.at(x - originX, y - originY), linear);
					}
				}
			});
			return patch;
		}
	}

	Document::Document() { reset(); }

	Document::~Document() { settlePaint(); }

	void Document::reset() {
		settlePaint();
		mLayers.clear();
		mActive = nullptr;
		mBackground = {};
		mFrame = {};
		mStrokeLayer = nullptr;
		mStrokeTiles.removeAll();
		mStrokeMasks.removeAll();
		mPreviewMasks.removeAll();
		mSmudgeCarry.clear();
		addLayer(nextLayerName());
		mHistory.reset("New file", state());
		resetView();
	}

	std::string Document::uniqueLayerName(const std::string& name) const {
		std::string base = name;
		const size_t space = base.find_last_of(' ');
		if (
			space != std::string::npos && space + 1 < base.size() &&
			base.find_first_not_of("0123456789", space + 1) == std::string::npos
		) {
			base.resize(space);
		}

		long highest = 0;
		for (const auto& layer : mLayers) {
			const std::string& existing = layer->name();
			if (existing.compare(0, base.size(), base) != 0) continue;
			highest = std::max(highest, std::strtol(existing.c_str() + base.size(), nullptr, 10));
		}
		return base + " " + std::to_string(highest + 1);
	}

	size_t Document::indexOf(const Layer& layer) const {
		for (size_t index = 0; index < mLayers.size(); index++) {
			if (mLayers[index].get() == &layer) return index;
		}
		return mLayers.size();
	}

	Layer* Document::duplicateLayer(const Layer& source) {
		settlePaint();
		const size_t index = indexOf(source);
		if (index == mLayers.size() || !canAddLayer()) return nullptr;

		Layer::State state = source.state();
		state.id = mNextLayerId++;
		state.properties.name = uniqueLayerName(source.name());
		state.properties.selected = false;
		state.properties.locked = false;

		auto copy = std::make_unique<Layer>(state.id, state.properties.name);
		copy->restore(state);
		Layer* added = copy.get();
		mLayers.insert(mLayers.begin() + (std::ptrdiff_t) index + 1, std::move(copy));
		mActive = added;
		return added;
	}

	bool Document::renameLayer(Layer& layer, const std::string& name) {
		std::string sanitized;
		for (const char character : name) {
			if (character == '\n') continue;
			sanitized += character == '"' ? ' ' : character;
		}
		if (sanitized.size() > kMaximumLayerNameLength) sanitized.resize(kMaximumLayerNameLength);
		if (sanitized == layer.name()) return false;

		layer.setName(sanitized);
		return true;
	}

	bool Document::moveActiveLayer(int4 offset) {
		if (!mActive) return false;
		const size_t index = indexOf(*mActive);
		const auto target = (std::ptrdiff_t) index + offset;
		if (target < 0 || target >= (std::ptrdiff_t) mLayers.size()) return false;

		const auto from = mLayers.begin() + (std::ptrdiff_t) index;
		const auto to = mLayers.begin() + target;
		if (offset > 0) std::rotate(from, from + 1, to + 1);
		else std::rotate(to, from, from + 1);
		mLayers.front()->setClipped(false);
		return true;
	}

	Layer* Document::addLayer(const std::string& name) {
		if (!canAddLayer()) return nullptr;
		const bool clipped = mActive && mActive->clipped();
		auto layer = std::make_unique<Layer>(mNextLayerId++, name);
		layer->setClipped(clipped);
		Layer* added = layer.get();

		auto position = mLayers.end();
		for (auto it = mLayers.begin(); it != mLayers.end(); ++it) {
			if (it->get() == mActive) position = it + 1;
		}
		mLayers.insert(position, std::move(layer));
		mActive = added;
		return added;
	}

	void Document::removeActiveLayer() {
		settlePaint();
		if (mLayers.size() <= 1 || !mActive) return;
		if (mStrokeLayer == mActive) finishStroke();

		for (auto it = mLayers.begin(); it != mLayers.end(); ++it) {
			if (it->get() != mActive) continue;
			const auto removed = it - mLayers.begin();
			mLayers.erase(it);
			mActive = mLayers[(size_t) (removed > 0 ? removed - 1 : 0)].get();
			return;
		}
	}

	void Document::setActiveLayer(Layer* layer) {
		if (!layer) return;
		mActive = layer;
	}

	bool Document::selectOnly(Layer& layer) {
		bool changed = mActive != &layer;
		for (const auto& candidate : mLayers) {
			changed |= candidate->selected();
			candidate->setSelected(false);
		}
		mActive = &layer;
		return changed;
	}

	bool Document::selectAdjacent(int4 offset) {
		if (!mActive) return false;
		const auto target =
			std::clamp((std::ptrdiff_t) indexOf(*mActive) + offset, (std::ptrdiff_t) 0, (std::ptrdiff_t) mLayers.size() - 1);
		return selectOnly(*mLayers[(size_t) target]);
	}

	bool Document::selectAllLayers() {
		bool changed = false;
		for (const auto& layer : mLayers) {
			changed |= !layer->selected() && layer.get() != mActive;
			layer->setSelected(true);
		}
		return changed;
	}

	bool Document::toggleLayerSelection(Layer& layer) {
		if (&layer == mActive) {
			if (selectedCount() <= 1) return false;
			layer.setSelected(false);
			for (const auto& candidate : mLayers) {
				if (candidate->selected()) {
					mActive = candidate.get();
					break;
				}
			}
			return true;
		}

		if (mActive) mActive->setSelected(true);
		layer.setSelected(!layer.selected());
		return true;
	}

	bool Document::selectRangeTo(Layer& layer) {
		if (!mActive) return selectOnly(layer);
		const size_t from = std::min(indexOf(*mActive), indexOf(layer));
		const size_t to = std::max(indexOf(*mActive), indexOf(layer));

		bool changed = false;
		for (size_t index = 0; index < mLayers.size(); index++) {
			const bool inside = index >= from && index <= to;
			changed |= mLayers[index]->selected() != inside;
			mLayers[index]->setSelected(inside);
		}
		return changed;
	}

	size_t Document::selectedCount() const {
		size_t count = 0;
		for (const auto& layer : mLayers) {
			if (layer->selected() || layer.get() == mActive) count++;
		}
		return count;
	}

	std::vector<Layer*> Document::mergeCandidates() const {
		std::vector<Layer*> layers = targetLayers();
		if (layers.size() != 1) return layers;

		Layer* below = layerBelow(*layers.front());
		if (!below) return {};
		return { below, layers.front() };
	}

	bool Document::bakeOpacity(Layer& layer) {
		settlePaint();
		const halnf opacity = layer.opacity();
		if (opacity >= 1.0f) return false;

		std::vector<TileCoord> coords;
		layer.forEachTile([&coords](const TileCoord& coord, Tile*) { coords.push_back(coord); });
		for (const TileCoord& coord : coords) {
			uint1* pixels = layer.tileAt(coord)->pixels();
			for (ualni index = 0; index < (ualni) kTileSize * kTileSize * 4; index++)
				pixels[index] = (uint1) std::lround(pixels[index] * opacity);
			layer.tileAt(coord)->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
		}
		layer.setOpacity(1.0f);
		return true;
	}

	bool Document::mergeLayers(const std::vector<Layer*>& requested) {
		settlePaint();
		std::vector<Layer*> layers = requested;
		std::sort(layers.begin(), layers.end(), [this](const Layer* left, const Layer* right) {
			return indexOf(*left) < indexOf(*right);
		});
		if (layers.size() < 2) return false;

		Layer& target = *layers.front();
		bakeOpacity(target);
		const Layer::Properties kept = target.properties();
		target.setBlend(BlendMode::Normal);
		target.setClipped(false);
		std::vector<const Layer*> stack(layers.begin(), layers.end());

		Map<ualni, TileCoord> coords;
		for (const Layer* layer : stack)
			layer->forEachTile([&coords](const TileCoord& coord, Tile*) { coords.put(coord.key(), coord); });

		Layer merged(target.id(), target.name());
		for (auto it : coords) {
			Tile* tile = merged.tileAt(it->val);
			Compositor::flatten(stack, it->val, *tile);
			if (tile->isTransparent()) merged.removeTile(it->val);
		}

		Layer::State state = merged.state();
		state.properties = kept;
		target.restore(state);

		for (size_t index = 1; index < layers.size(); index++) {
			if (mStrokeLayer == layers[index]) finishStroke();
			mLayers.erase(mLayers.begin() + (std::ptrdiff_t) indexOf(*layers[index]));
		}

		for (const auto& layer : mLayers)
			layer->setSelected(false);
		mActive = &target;
		return true;
	}

	bool Document::mergeVisibleLayers() {
		std::vector<Layer*> layers;
		for (const auto& layer : mLayers) {
			if (layer->visible()) layers.push_back(layer.get());
		}
		return mergeLayers(layers);
	}

	void Document::clearActiveLayer() {
		settlePaint();
		if (mActive) mActive->clear();
	}

	void Document::setLayerVisible(Layer& layer, bool visible) { layer.setVisible(visible); }

	void Document::setLayerOpacity(Layer& layer, halnf opacity) {
		layer.setOpacity(opacity < 0 ? 0 : (opacity > 1 ? 1 : opacity));
	}

	std::vector<Layer*> Document::targetLayers() const {
		std::vector<Layer*> layers;
		for (const auto& layer : mLayers) {
			if (layer.get() == mActive || layer->selected()) layers.push_back(layer.get());
		}
		return layers;
	}

	Layer* Document::layerBelow(const Layer& layer) const {
		for (size_t index = 1; index < mLayers.size(); index++) {
			if (mLayers[index].get() == &layer) return mLayers[index - 1].get();
		}
		return nullptr;
	}

	Layer* Document::layerAbove(const Layer& layer) const {
		for (size_t index = 0; index + 1 < mLayers.size(); index++) {
			if (mLayers[index].get() == &layer) return mLayers[index + 1].get();
		}
		return nullptr;
	}

	bool Document::toggleVisible() {
		const bool visible = mActive && !mActive->visible();
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			changed |= layer->visible() != visible;
			layer->setVisible(visible);
		}
		return changed;
	}

	bool Document::toggleClip() {
		const bool clipped = mActive && !mActive->clipped();
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			if (clipped && !layerBelow(*layer)) continue;
			changed |= layer->clipped() != clipped;
			layer->setClipped(clipped);
		}
		return changed;
	}

	bool Document::toggleLock() {
		const bool locked = mActive && !mActive->locked();
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			changed |= layer->locked() != locked;
			layer->setLocked(locked);
		}
		return changed;
	}

	bool Document::toggleAlphaLock() {
		const bool alphaLocked = mActive && !mActive->alphaLocked();
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			changed |= layer->alphaLocked() != alphaLocked;
			layer->setAlphaLocked(alphaLocked);
		}
		return changed;
	}

	bool Document::cycleBlend(int4 direction) {
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			if (!canEdit(*layer)) continue;
			layer->setBlend(direction > 0 ? nextBlendMode(layer->blend()) : previousBlendMode(layer->blend()));
			changed = true;
		}
		return changed;
	}

	bool Document::resetBlend() { return setBlend(BlendMode::Normal); }

	bool Document::setBlend(BlendMode blend) {
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			if (!canEdit(*layer) || layer->blend() == blend) continue;
			layer->setBlend(blend);
			changed = true;
		}
		return changed;
	}

	bool Document::changeOpacity(halnf delta) {
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			if (!canEdit(*layer)) continue;
			const halnf before = layer->opacity();
			setLayerOpacity(*layer, before + delta);
			changed |= layer->opacity() != before;
		}
		return changed;
	}

	bool Document::setOpacity(halnf opacity) {
		bool changed = false;
		for (Layer* layer : targetLayers()) {
			if (!canEdit(*layer)) continue;
			const halnf before = layer->opacity();
			setLayerOpacity(*layer, opacity);
			changed |= layer->opacity() != before;
		}
		return changed;
	}

	DocumentState Document::state() const {
		settlePaint();
		return stateWithoutSettling();
	}

	DocumentState Document::stateWithoutSettling() const {
		DocumentState snapshot;
		snapshot.layers.reserve(mLayers.size());
		for (const auto& layer : mLayers)
			snapshot.layers.push_back(layer->state());
		snapshot.active = mActive ? mActive->id() : 0;
		snapshot.background = mBackground;
		snapshot.frame = mFrame;
		snapshot.selection = mSelection;
		return snapshot;
	}

	RGBA HueColor::rgba(halnf alpha) const {
		const halnf hue = std::fmod(std::fmod(hueDegrees, 360.0f) + 360.0f, 360.0f) / 60.0f;
		const halnf chroma = value * saturation;
		const halnf secondary = chroma * (1.0f - std::fabs(std::fmod(hue, 2.0f) - 1.0f));
		const halnf base = value - chroma;

		halnf r = 0;
		halnf g = 0;
		halnf b = 0;
		switch ((int4) hue) {
			case 0: r = chroma, g = secondary; break;
			case 1: r = secondary, g = chroma; break;
			case 2: g = chroma, b = secondary; break;
			case 3: g = secondary, b = chroma; break;
			case 4: r = secondary, b = chroma; break;
			default: r = chroma, b = secondary; break;
		}
		return { r + base, g + base, b + base, alpha };
	}

	void CanvasFrame::clamp() {
		left = std::clamp(left, -kCoordinateLimit, kCoordinateLimit - 1);
		top = std::clamp(top, -kCoordinateLimit, kCoordinateLimit - 1);
		right = std::max(left + 1, std::clamp(right, -kCoordinateLimit, kCoordinateLimit));
		bottom = std::max(top + 1, std::clamp(bottom, -kCoordinateLimit, kCoordinateLimit));
	}

	RGBA Document::sampleColor(Vec2F world, bool allLayers) const {
		settlePaint();
		const int4 px = (int4) std::floor(world.x);
		const int4 py = (int4) std::floor(world.y);
		const TileCoord coord = { floordiv(px, kTileSize), floordiv(py, kTileSize) };
		const ualni index = (ualni) (py - coord.y * kTileSize) * kTileSize + (ualni) (px - coord.x * kTileSize);

		const uint1* pixel = nullptr;
		Tile flattened;
		if (allLayers) {
			std::vector<const Layer*> visible;
			for (const auto& layer : mLayers)
				if (layer->visible()) visible.push_back(layer.get());
			if (visible.empty()) return { 0, 0, 0, 0 };
			Compositor::flatten(visible, coord, flattened);
			pixel = flattened.pixels() + index * 4;
		} else {
			if (!mActive) return { 0, 0, 0, 0 };
			const Tile* tile = mActive->tileIfExists(coord);
			if (!tile) return { 0, 0, 0, 0 };
			pixel = tile->pixels() + index * 4;
		}
		const halnf alpha = pixel[3] / 255.0f;
		if (alpha <= 0) return { 0, 0, 0, 0 };
		return { std::min(1.0f, pixel[0] / 255.0f / alpha),
						 std::min(1.0f, pixel[1] / 255.0f / alpha),
						 std::min(1.0f, pixel[2] / 255.0f / alpha),
						 alpha };
	}

	Document::FloodRegion Document::floodRegion(Vec2F world, const BucketSettings& settings, bool refuseLeaks) const {
		FloodRegion region;
		const int4 seedX = (int4) std::floor(world.x);
		const int4 seedY = (int4) std::floor(world.y);

		int4 left = 0;
		int4 top = 0;
		int4 right = 0;
		int4 bottom = 0;
		const bool framed = !mFrame.infinite && !mFrame.isEmpty();
		if (framed) {
			left = mFrame.left;
			top = mFrame.top;
			right = mFrame.right;
			bottom = mFrame.bottom;
		} else {
			if (!contentBounds(left, top, right, bottom)) {
				region.result = BucketResult::Empty;
				return region;
			}
			left -= kBucketOpenMargin;
			top -= kBucketOpenMargin;
			right += kBucketOpenMargin;
			bottom += kBucketOpenMargin;
		}
		if (seedX < left || seedX >= right || seedY < top || seedY >= bottom) {
			region.result = BucketResult::Leak;
			return region;
		}
		const int4 width = right - left;
		const int4 height = bottom - top;
		if ((int8) width * height > kBucketMaximumPixels) {
			region.result = BucketResult::TooBig;
			return region;
		}

		std::vector<const Layer*> sources;
		if (settings.allLayers) {
			for (const auto& layer : mLayers)
				if (layer->visible()) sources.push_back(layer.get());
		} else {
			sources.push_back(mActive);
		}

		std::vector<uint1> sample((size_t) width * height * 4, 0);
		Tile flattened;
		for (int4 ty = floordiv(top, kTileSize); ty <= floordiv(bottom - 1, kTileSize); ty++) {
			for (int4 tx = floordiv(left, kTileSize); tx <= floordiv(right - 1, kTileSize); tx++) {
				const TileCoord coord = { tx, ty };
				const uint1* pixels = nullptr;
				if (sources.size() == 1) {
					const Tile* tile = sources.front()->tileIfExists(coord);
					if (!tile) continue;
					pixels = tile->pixels();
				} else {
					bool any = false;
					for (const Layer* layer : sources)
						any = any || layer->tileIfExists(coord) != nullptr;
					if (!any) continue;
					Compositor::flatten(sources, coord, flattened);
					pixels = flattened.pixels();
				}
				const int4 x0 = std::max(left, tx * kTileSize);
				const int4 x1 = std::min(right, (tx + 1) * kTileSize);
				const int4 y0 = std::max(top, ty * kTileSize);
				const int4 y1 = std::min(bottom, (ty + 1) * kTileSize);
				for (int4 y = y0; y < y1; y++) {
					const uint1* from = pixels + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x0 - tx * kTileSize)) * 4;
					uint1* to = sample.data() + ((size_t) (y - top) * width + (size_t) (x0 - left)) * 4;
					std::memcpy(to, from, (size_t) (x1 - x0) * 4);
				}
			}
		}

		const auto at = [&](int4 x, int4 y) { return sample.data() + ((size_t) y * width + (size_t) x) * 4; };
		const uint1* seed = at(seedX - left, seedY - top);
		const int4 seedPixel[4] = { seed[0], seed[1], seed[2], seed[3] };
		const int4 limit = (int4) std::lround(std::clamp(settings.threshold, 0.0f, 1.0f) * 255.0f);
		const auto matches = [&](int4 x, int4 y) {
			const uint1* pixel = at(x, y);
			if (settings.onlyAlpha) return std::abs(pixel[3] - seedPixel[3]) <= limit;
			for (int4 channel = 0; channel < 4; channel++)
				if (std::abs(pixel[channel] - seedPixel[channel]) > limit) return false;
			return true;
		};

		std::vector<uint1> mask((size_t) width * height, 0);
		std::vector<std::pair<int4, int4>> stack = { { seedX - left, seedY - top } };
		bool leaked = false;
		while (!stack.empty()) {
			const auto [x, y] = stack.back();
			stack.pop_back();
			if (mask[(size_t) y * width + x] || !matches(x, y)) continue;
			int4 from = x;
			while (from > 0 && !mask[(size_t) y * width + from - 1] && matches(from - 1, y))
				from--;
			int4 to = x;
			while (to + 1 < width && !mask[(size_t) y * width + to + 1] && matches(to + 1, y))
				to++;
			if (from == 0 || to == width - 1 || y == 0 || y == height - 1) leaked = true;
			for (int4 run = from; run <= to; run++)
				mask[(size_t) y * width + run] = 1;
			for (const int4 row : { y - 1, y + 1 }) {
				if (row < 0 || row >= height) continue;
				for (int4 run = from; run <= to; run++)
					if (!mask[(size_t) row * width + run] && matches(run, row)) stack.push_back({ run, row });
			}
		}
		if (leaked && !framed && refuseLeaks) {
			region.result = BucketResult::Leak;
			return region;
		}

		for (int4 grow = 0; grow < settings.spread; grow++) {
			std::vector<uint1> next = mask;
			for (int4 y = 0; y < height; y++)
				for (int4 x = 0; x < width; x++) {
					if (mask[(size_t) y * width + x]) continue;
					const bool touches =
						(x > 0 && mask[(size_t) y * width + x - 1]) || (x + 1 < width && mask[(size_t) y * width + x + 1]) ||
						(y > 0 && mask[(size_t) (y - 1) * width + x]) || (y + 1 < height && mask[(size_t) (y + 1) * width + x]);
					if (touches) next[(size_t) y * width + x] = 1;
				}
			mask = std::move(next);
		}

		region.result = BucketResult::Filled;
		region.left = left;
		region.top = top;
		region.width = width;
		region.height = height;
		region.mask = std::move(mask);
		return region;
	}

	BucketResult Document::bucketFill(Vec2F world, const RGBA& color, const BucketSettings& settings) {
		settlePaint();
		if (!mActive || !canEdit(*mActive)) return BucketResult::Locked;
		FloodRegion region = floodRegion(world, settings);
		if (region.result != BucketResult::Filled) return region.result;
		const int4 left = region.left;
		const int4 top = region.top;
		const int4 width = region.width;
		const int4 right = left + region.width;
		const int4 bottom = top + region.height;
		const std::vector<uint1>& mask = region.mask;
		const uint1 source[4] = {
			(uint1) std::lround(std::clamp(color.r * color.a, 0.0f, 1.0f) * 255.0f),
			(uint1) std::lround(std::clamp(color.g * color.a, 0.0f, 1.0f) * 255.0f),
			(uint1) std::lround(std::clamp(color.b * color.a, 0.0f, 1.0f) * 255.0f),
			(uint1) std::lround(std::clamp(color.a, 0.0f, 1.0f) * 255.0f),
		};
		const bool keepAlpha = mActive->alphaLocked();
		const bool clipped = mSelection.isActive();
		for (int4 ty = floordiv(top, kTileSize); ty <= floordiv(bottom - 1, kTileSize); ty++) {
			for (int4 tx = floordiv(left, kTileSize); tx <= floordiv(right - 1, kTileSize); tx++) {
				const int4 x0 = std::max(left, tx * kTileSize);
				const int4 x1 = std::min(right, (tx + 1) * kTileSize);
				const int4 y0 = std::max(top, ty * kTileSize);
				const int4 y1 = std::min(bottom, (ty + 1) * kTileSize);
				bool any = false;
				for (int4 y = y0; y < y1 && !any; y++)
					for (int4 x = x0; x < x1 && !any; x++)
						any = mask[(size_t) (y - top) * width + (x - left)] != 0;
				if (!any) continue;
				const TileCoord coord = { tx, ty };
				if (keepAlpha && !mActive->tileIfExists(coord)) continue;
				Tile* tile = mActive->tileAt(coord);
				uint1* pixels = tile->pixels();
				for (int4 y = y0; y < y1; y++)
					for (int4 x = x0; x < x1; x++) {
						if (!mask[(size_t) (y - top) * width + (x - left)]) continue;
						uint1* pixel = pixels + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x - tx * kTileSize)) * 4;
						const halnf selected = clipped ? mSelection.coverage(x, y) / 255.0f : 1.0f;
						if (selected <= 0) continue;
						if (selected < 1) {
							const halnf rgb[3] = { color.r, color.g, color.b };
							const halnf alpha = std::clamp(color.a, 0.0f, 1.0f) * selected;
							for (int4 channel = 0; channel < 3; channel++)
								pixel[channel] = (uint1) std::lround(
									std::clamp(rgb[channel], 0.0f, 1.0f) * alpha * 255.0f + pixel[channel] * (1 - alpha)
								);
							if (!keepAlpha) pixel[3] = (uint1) std::lround(alpha * 255.0f + pixel[3] * (1 - alpha));
							continue;
						}
						if (keepAlpha) {
							const halnf alpha = pixel[3] / 255.0f;
							const halnf rgb[3] = { color.r, color.g, color.b };
							for (int4 channel = 0; channel < 3; channel++)
								pixel[channel] = (uint1) std::lround(std::clamp(rgb[channel], 0.0f, 1.0f) * alpha * 255.0f);
							continue;
						}
						const uint4 keep = 255u - source[3];
						for (int4 channel = 0; channel < 4; channel++)
							pixel[channel] = (uint1) std::min(255u, source[channel] + (pixel[channel] * keep + 127) / 255);
					}
				tile->markDirty(
					{ (halnf) (x0 - tx * kTileSize), (halnf) (y0 - ty * kTileSize), (halnf) (x1 - x0), (halnf) (y1 - y0) }
				);
			}
		}
		return BucketResult::Filled;
	}

	halnf Document::gradientPosition(Vec2F point, Vec2F from, Vec2F to, GradientShape shape) {
		const Vec2F axis = to - from;
		const halnf length2 = axis.x * axis.x + axis.y * axis.y;
		if (length2 <= 0) return 1;
		const Vec2F offset = point - from;
		if (shape == GradientShape::Radial)
			return std::clamp(std::sqrt((offset.x * offset.x + offset.y * offset.y) / length2), 0.0f, 1.0f);
		return std::clamp((offset.x * axis.x + offset.y * axis.y) / length2, 0.0f, 1.0f);
	}

	GradientResult Document::drawGradient(
		Vec2F fromWorld, Vec2F toWorld, const RGBA& start, const RGBA& end, const GradientSettings& settings
	) {
		settlePaint();
		int4 areaLeft = mFrame.left, areaTop = mFrame.top, areaRight = mFrame.right, areaBottom = mFrame.bottom;
		const bool clipped = mSelection.isActive();
		if (clipped && mSelection.bounds(areaLeft, areaTop, areaRight, areaBottom)) {
			if (!mFrame.infinite && !mFrame.isEmpty()) {
				areaLeft = std::max(areaLeft, mFrame.left);
				areaTop = std::max(areaTop, mFrame.top);
				areaRight = std::min(areaRight, mFrame.right);
				areaBottom = std::min(areaBottom, mFrame.bottom);
			}
		} else if (mFrame.infinite || mFrame.isEmpty()) {
			return GradientResult::NoFrame;
		}
		if (!mActive || !canEdit(*mActive)) return GradientResult::Locked;
		const bool keepAlpha = mActive->alphaLocked();
		const RGBA last = settings.toTransparent ? RGBA(start.r, start.g, start.b, 0) : end;
		for (int4 ty = floordiv(areaTop, kTileSize); ty <= floordiv(areaBottom - 1, kTileSize); ty++) {
			for (int4 tx = floordiv(areaLeft, kTileSize); tx <= floordiv(areaRight - 1, kTileSize); tx++) {
				const TileCoord coord = { tx, ty };
				if ((settings.eraser || keepAlpha) && !mActive->tileIfExists(coord)) continue;
				const int4 x0 = std::max(areaLeft, tx * kTileSize);
				const int4 x1 = std::min(areaRight, (tx + 1) * kTileSize);
				const int4 y0 = std::max(areaTop, ty * kTileSize);
				const int4 y1 = std::min(areaBottom, (ty + 1) * kTileSize);
				Tile* tile = mActive->tileAt(coord);
				uint1* pixels = tile->pixels();
				for (int4 y = y0; y < y1; y++) {
					for (int4 x = x0; x < x1; x++) {
						const halnf t =
							gradientPosition({ (halnf) x + 0.5f, (halnf) y + 0.5f }, fromWorld, toWorld, settings.shape);
						const halnf selected = clipped ? mSelection.coverage(x, y) / 255.0f : 1.0f;
						if (selected <= 0) continue;
						const halnf alpha = (start.a + (last.a - start.a) * t) * selected;
						uint1* pixel = pixels + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x - tx * kTileSize)) * 4;
						if (settings.eraser) {
							const halnf keep = 1.0f - std::clamp(alpha, 0.0f, 1.0f);
							for (int4 channel = 0; channel < 4; channel++)
								pixel[channel] = (uint1) std::lround(pixel[channel] * keep);
							continue;
						}
						halnf rgb[3] = { start.r + (end.r - start.r) * t,
														 start.g + (end.g - start.g) * t,
														 start.b + (end.b - start.b) * t };
						if (settings.toTransparent) {
							rgb[0] = start.r;
							rgb[1] = start.g;
							rgb[2] = start.b;
						}
						if (keepAlpha) {
							const halnf existing = pixel[3] / 255.0f;
							for (int4 channel = 0; channel < 3; channel++) {
								const halnf current = existing > 0 ? pixel[channel] / 255.0f / existing : 0;
								const halnf mixed = current + (std::clamp(rgb[channel], 0.0f, 1.0f) - current) * alpha;
								pixel[channel] = (uint1) std::lround(mixed * existing * 255.0f);
							}
							continue;
						}
						const halnf source = std::clamp(alpha, 0.0f, 1.0f);
						for (int4 channel = 0; channel < 3; channel++)
							pixel[channel] = (uint1) std::lround(
								std::clamp(rgb[channel], 0.0f, 1.0f) * source * 255.0f + pixel[channel] * (1 - source)
							);
						pixel[3] = (uint1) std::lround(source * 255.0f + pixel[3] * (1 - source));
					}
				}
				tile->markDirty(
					{ (halnf) (x0 - tx * kTileSize), (halnf) (y0 - ty * kTileSize), (halnf) (x1 - x0), (halnf) (y1 - y0) }
				);
			}
		}
		return GradientResult::Drawn;
	}

	bool Document::beginMove(bool duplicate) {
		settlePaint();
		if (!mActive || !canEdit(*mActive) || mActive->tileCount() == 0) return false;
		mMoveSource = mActive->state();
		mMoveDuplicate = duplicate;
		mMoveSelection = mSelection;
		mMoveSelectionOnly = mSelection.isActive();
		mMoveX = mMoveY = 0;
		return true;
	}

	void Document::moveTo(int4 dx, int4 dy) {
		if (!mMoveSource || !mActive || (dx == mMoveX && dy == mMoveY)) return;
		mMoveX = dx;
		mMoveY = dy;
		mActive->restore(*mMoveSource);
		mSelection = mMoveSelection;
		const Selection& carried = mMoveSelection;
		if (!mMoveDuplicate) {
			if (mMoveSelectionOnly) {
				std::vector<TileCoord> coords;
				mActive->forEachTile([&coords](const TileCoord& coord, Tile*) { coords.push_back(coord); });
				for (const TileCoord& coord : coords) {
					Tile* tile = nullptr;
					for (int4 y = 0; y < kTileSize; y++)
						for (int4 x = 0; x < kTileSize; x++) {
							const halnf cut = carried.coverage(coord.x * kTileSize + x, coord.y * kTileSize + y) / 255.0f;
							if (cut <= 0) continue;
							if (!tile) tile = mActive->tileAt(coord);
							uint1* pixel = tile->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
							for (int4 channel = 0; channel < 4; channel++)
								pixel[channel] = (uint1) std::lround(pixel[channel] * (1 - cut));
						}
					if (tile) tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
				}
			} else {
				mActive->clear();
			}
		}
		const bool keepAlpha = mActive->alphaLocked() && !mMoveDuplicate;
		for (auto it : mMoveSource->tiles) {
			const TileCoord source = it->val.coord;
			const uint1* from = it->val.tile->pixels();
			const int4 baseX = source.x * kTileSize + dx;
			const int4 baseY = source.y * kTileSize + dy;
			for (int4 ty = floordiv(baseY, kTileSize); ty <= floordiv(baseY + kTileSize - 1, kTileSize); ty++) {
				for (int4 tx = floordiv(baseX, kTileSize); tx <= floordiv(baseX + kTileSize - 1, kTileSize); tx++) {
					const int4 x0 = std::max(baseX, tx * kTileSize);
					const int4 x1 = std::min(baseX + kTileSize, (tx + 1) * kTileSize);
					const int4 y0 = std::max(baseY, ty * kTileSize);
					const int4 y1 = std::min(baseY + kTileSize, (ty + 1) * kTileSize);
					bool any = false;
					for (int4 y = y0; y < y1 && !any; y++)
						for (int4 x = x0; x < x1 && !any; x++)
							any = from[((size_t) (y - baseY) * kTileSize + (size_t) (x - baseX)) * 4 + 3] != 0;
					if (!any || keepAlpha) continue;
					Tile* tile = mActive->tileAt({ tx, ty });
					uint1* to = tile->pixels();
					for (int4 y = y0; y < y1; y++) {
						for (int4 x = x0; x < x1; x++) {
							const uint1* source = from + ((size_t) (y - baseY) * kTileSize + (size_t) (x - baseX)) * 4;
							if (source[3] == 0) continue;
							uint4 above[4] = { source[0], source[1], source[2], source[3] };
							if (mMoveSelectionOnly) {
								const uint4 take = carried.coverage(x - dx, y - dy);
								if (take == 0) continue;
								for (int4 channel = 0; channel < 4; channel++)
									above[channel] = (above[channel] * take + 127) / 255;
							}
							uint1* below = to + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x - tx * kTileSize)) * 4;
							const uint4 keep = 255u - above[3];
							for (int4 channel = 0; channel < 4; channel++)
								below[channel] = (uint1) std::min(255u, above[channel] + (below[channel] * keep + 127) / 255);
						}
					}
					tile->markDirty(
						{ (halnf) (x0 - tx * kTileSize), (halnf) (y0 - ty * kTileSize), (halnf) (x1 - x0), (halnf) (y1 - y0) }
					);
				}
			}
		}
		if (mMoveSelectionOnly) {
			const Selection& origin = mMoveSelection;
			int4 left = 0, top = 0, right = 0, bottom = 0;
			if (origin.bounds(left, top, right, bottom)) {
				std::vector<uint1> shifted((size_t) (right - left) * (bottom - top));
				for (int4 y = top; y < bottom; y++)
					for (int4 x = left; x < right; x++)
						shifted[(size_t) (y - top) * (right - left) + (x - left)] = origin.coverage(x, y);
				mSelection.clear();
				mSelection.addMask(left + dx, top + dy, right - left, shifted, SelectionMode::Replace);
			}
		}
	}

	bool Document::finishMove() {
		if (!mMoveSource) return false;
		const bool moved = mMoveX != 0 || mMoveY != 0;
		if (!moved) {
			mActive->restore(*mMoveSource);
			mSelection = mMoveSelection;
		}
		mMoveSource.reset();
		return moved;
	}

	void Document::cancelMove() {
		if (!mMoveSource) return;
		if (mActive) mActive->restore(*mMoveSource);
		mSelection = mMoveSelection;
		mMoveSource.reset();
	}

	Vec2F LayerTransform::apply(Vec2F point) const {
		const Vec2F local = { (point.x - pivot.x) * scale.x, (point.y - pivot.y) * scale.y };
		const halnf c = std::cos(rotation);
		const halnf s = std::sin(rotation);
		return { pivot.x + offset.x + local.x * c - local.y * s, pivot.y + offset.y + local.x * s + local.y * c };
	}

	Vec2F LayerTransform::invert(Vec2F point) const {
		const Vec2F moved = { point.x - pivot.x - offset.x, point.y - pivot.y - offset.y };
		const halnf c = std::cos(rotation);
		const halnf s = std::sin(rotation);
		const Vec2F turned = { moved.x * c + moved.y * s, -moved.x * s + moved.y * c };
		return { pivot.x + (scale.x != 0 ? turned.x / scale.x : 0.0f),
						 pivot.y + (scale.y != 0 ? turned.y / scale.y : 0.0f) };
	}

	bool LayerTransform::isIdentity() const {
		return scale.x == 1 && scale.y == 1 && rotation == 0 && offset.x == 0 && offset.y == 0;
	}

	bool Document::layerBounds(const Layer& layer, int4& left, int4& top, int4& right, int4& bottom) const {
		bool any = false;
		layer.forEachTile([&](const TileCoord& coord, Tile* tile) {
			for (int4 y = 0; y < kTileSize; y++)
				for (int4 x = 0; x < kTileSize; x++) {
					if (tile->pixels()[((ualni) y * kTileSize + (ualni) x) * 4 + 3] == 0) continue;
					const int4 px = coord.x * kTileSize + x;
					const int4 py = coord.y * kTileSize + y;
					if (!any) {
						left = right = px;
						top = bottom = py;
						any = true;
					}
					left = std::min(left, px);
					top = std::min(top, py);
					right = std::max(right, px + 1);
					bottom = std::max(bottom, py + 1);
				}
		});
		return any;
	}

	bool Document::beginTransform() {
		settlePaint();
		if (!mActive || !canEdit(*mActive)) return false;
		int4 bounds[4];
		if (!layerBounds(*mActive, bounds[0], bounds[1], bounds[2], bounds[3])) return false;
		mTransformSource = mActive->state();
		mTransformChanged = false;
		std::copy(bounds, bounds + 4, mTransformBounds);
		return true;
	}

	bool Document::transformBounds(int4& left, int4& top, int4& right, int4& bottom) const {
		if (!mTransformSource) return false;
		left = mTransformBounds[0];
		top = mTransformBounds[1];
		right = mTransformBounds[2];
		bottom = mTransformBounds[3];
		return true;
	}

	namespace {
		void sampleBilinear(const Map<ualni, Layer::TileSlot>& tiles, halnf x, halnf y, halnf out[4]) {
			const auto texel = [&tiles](int4 px, int4 py, halnf result[4]) {
				const TileCoord coord = { floordiv(px, kTileSize), floordiv(py, kTileSize) };
				auto found = tiles.presents(coord.key());
				if (!found.isValid()) {
					result[0] = result[1] = result[2] = result[3] = 0;
					return;
				}
				const uint1* pixel =
					tiles.getSlotVal(found).tile->pixels() +
					((size_t) (py - coord.y * kTileSize) * kTileSize + (size_t) (px - coord.x * kTileSize)) * 4;
				for (int4 channel = 0; channel < 4; channel++)
					result[channel] = pixel[channel];
			};
			const halnf fx = x - 0.5f;
			const halnf fy = y - 0.5f;
			const int4 x0 = (int4) std::floor(fx);
			const int4 y0 = (int4) std::floor(fy);
			const halnf tx = fx - (halnf) x0;
			const halnf ty = fy - (halnf) y0;
			halnf a[4], b[4], c[4], d[4];
			texel(x0, y0, a);
			texel(x0 + 1, y0, b);
			texel(x0, y0 + 1, c);
			texel(x0 + 1, y0 + 1, d);
			for (int4 channel = 0; channel < 4; channel++) {
				const halnf top = a[channel] + (b[channel] - a[channel]) * tx;
				const halnf bottom = c[channel] + (d[channel] - c[channel]) * tx;
				out[channel] = top + (bottom - top) * ty;
			}
		}
	}

	void Document::previewTransform(const LayerTransform& transform) {
		if (!mTransformSource || !mActive) return;
		mActive->restore(*mTransformSource);
		mTransformChanged = !transform.isIdentity();
		if (!mTransformChanged) return;
		mActive->clear();
		const halnf corners[4][2] = {
			{ (halnf) mTransformBounds[0], (halnf) mTransformBounds[1] },
			{ (halnf) mTransformBounds[2], (halnf) mTransformBounds[1] },
			{ (halnf) mTransformBounds[0], (halnf) mTransformBounds[3] },
			{ (halnf) mTransformBounds[2], (halnf) mTransformBounds[3] },
		};
		halnf minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
		for (const auto& corner : corners) {
			const Vec2F mapped = transform.apply({ corner[0], corner[1] });
			minX = std::min(minX, mapped.x);
			minY = std::min(minY, mapped.y);
			maxX = std::max(maxX, mapped.x);
			maxY = std::max(maxY, mapped.y);
		}
		const int4 left = (int4) std::floor(minX) - 1;
		const int4 top = (int4) std::floor(minY) - 1;
		const int4 right = (int4) std::ceil(maxX) + 1;
		const int4 bottom = (int4) std::ceil(maxY) + 1;
		if ((int8) (right - left) * (bottom - top) > kBucketMaximumPixels) return;
		for (int4 ty = floordiv(top, kTileSize); ty <= floordiv(bottom - 1, kTileSize); ty++) {
			for (int4 tx = floordiv(left, kTileSize); tx <= floordiv(right - 1, kTileSize); tx++) {
				const int4 x0 = std::max(left, tx * kTileSize);
				const int4 x1 = std::min(right, (tx + 1) * kTileSize);
				const int4 y0 = std::max(top, ty * kTileSize);
				const int4 y1 = std::min(bottom, (ty + 1) * kTileSize);
				Tile* tile = nullptr;
				for (int4 y = y0; y < y1; y++) {
					for (int4 x = x0; x < x1; x++) {
						const Vec2F source = transform.invert({ (halnf) x + 0.5f, (halnf) y + 0.5f });
						halnf color[4];
						sampleBilinear(mTransformSource->tiles, source.x, source.y, color);
						if (color[3] < 0.5f) continue;
						if (!tile) tile = mActive->tileAt({ tx, ty });
						uint1* pixel =
							tile->pixels() + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x - tx * kTileSize)) * 4;
						for (int4 channel = 0; channel < 4; channel++)
							pixel[channel] = (uint1) std::clamp((int4) std::lround(color[channel]), 0, 255);
					}
				}
				if (tile)
					tile->markDirty(
						{ (halnf) (x0 - tx * kTileSize), (halnf) (y0 - ty * kTileSize), (halnf) (x1 - x0), (halnf) (y1 - y0) }
					);
			}
		}
	}

	bool Document::finishTransform() {
		if (!mTransformSource) return false;
		const bool changed = mTransformChanged;
		if (!changed && mActive) mActive->restore(*mTransformSource);
		mTransformSource.reset();
		mTransformChanged = false;
		return changed;
	}

	void Document::cancelTransform() {
		if (!mTransformSource) return;
		if (mActive) mActive->restore(*mTransformSource);
		mTransformSource.reset();
	}

	bool Document::flipActiveLayer(bool horizontal) {
		if (!beginTransform()) return false;
		LayerTransform flip;
		flip.pivot = { (mTransformBounds[0] + mTransformBounds[2]) / 2.0f,
									 (mTransformBounds[1] + mTransformBounds[3]) / 2.0f };
		flip.scale = horizontal ? Vec2F{ -1, 1 } : Vec2F{ 1, -1 };
		previewTransform(flip);
		return finishTransform();
	}

	CropResult Document::cropTo(int4 left, int4 top, int4 right, int4 bottom) {
		settlePaint();
		if (right <= left || bottom <= top) return CropResult::NoRegion;
		for (const auto& layer : mLayers)
			if (layer->alphaLocked()) return CropResult::AlphaLocked;
		bool changed = setInfinite(false);
		changed |= setFrame(left, top, right, bottom);
		for (const auto& layer : mLayers) {
			std::vector<TileCoord> coords;
			layer->forEachTile([&coords](const TileCoord& coord, Tile*) { coords.push_back(coord); });
			for (const TileCoord& coord : coords) {
				const int4 x0 = coord.x * kTileSize;
				const int4 y0 = coord.y * kTileSize;
				const int4 x1 = x0 + kTileSize;
				const int4 y1 = y0 + kTileSize;
				if (x1 <= left || x0 >= right || y1 <= top || y0 >= bottom) {
					layer->removeTile(coord);
					changed = true;
					continue;
				}
				if (x0 >= left && x1 <= right && y0 >= top && y1 <= bottom) continue;
				const Tile* existing = layer->tileIfExists(coord);
				bool outside = false;
				for (int4 y = 0; y < kTileSize && !outside; y++)
					for (int4 x = 0; x < kTileSize && !outside; x++) {
						const int4 px = x0 + x;
						const int4 py = y0 + y;
						if (px >= left && px < right && py >= top && py < bottom) continue;
						outside = existing->pixels()[((ualni) y * kTileSize + (ualni) x) * 4 + 3] != 0;
					}
				if (!outside) continue;
				Tile* tile = layer->tileAt(coord);
				for (int4 y = 0; y < kTileSize; y++)
					for (int4 x = 0; x < kTileSize; x++) {
						const int4 px = x0 + x;
						const int4 py = y0 + y;
						if (px >= left && px < right && py >= top && py < bottom) continue;
						uint1* pixel = tile->pixels() + ((ualni) y * kTileSize + (ualni) x) * 4;
						pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
					}
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
				changed = true;
			}
		}
		return changed ? CropResult::Cropped : CropResult::Unchanged;
	}

	BucketResult Document::magicWand(Vec2F world, const BucketSettings& settings, SelectionMode mode) {
		FloodRegion region = floodRegion(world, settings, false);
		if (region.result != BucketResult::Filled) return region.result;
		std::vector<uint1> coverage(region.mask.size());
		for (size_t index = 0; index < coverage.size(); index++)
			coverage[index] = region.mask[index] ? 255 : 0;
		mSelection.addMask(region.left, region.top, region.width, coverage, mode);
		return BucketResult::Filled;
	}

	Document::SelectionEditResult Document::fillSelection(const RGBA& color) {
		settlePaint();
		if (!mSelection.isActive()) return SelectionEditResult::NoSelection;
		if (mSelection.isInverted()) return SelectionEditResult::Inverted;
		if (!mActive || !canEdit(*mActive)) return SelectionEditResult::Locked;
		int4 left = 0, top = 0, right = 0, bottom = 0;
		if (!mSelection.bounds(left, top, right, bottom)) return SelectionEditResult::NoSelection;
		const bool keepAlpha = mActive->alphaLocked();
		const halnf rgb[3] = { std::clamp(color.r, 0.0f, 1.0f),
													 std::clamp(color.g, 0.0f, 1.0f),
													 std::clamp(color.b, 0.0f, 1.0f) };
		for (int4 ty = floordiv(top, kTileSize); ty <= floordiv(bottom - 1, kTileSize); ty++)
			for (int4 tx = floordiv(left, kTileSize); tx <= floordiv(right - 1, kTileSize); tx++) {
				const TileCoord coord = { tx, ty };
				if (keepAlpha && !mActive->tileIfExists(coord)) continue;
				const int4 x0 = std::max(left, tx * kTileSize), x1 = std::min(right, (tx + 1) * kTileSize);
				const int4 y0 = std::max(top, ty * kTileSize), y1 = std::min(bottom, (ty + 1) * kTileSize);
				Tile* tile = nullptr;
				for (int4 y = y0; y < y1; y++)
					for (int4 x = x0; x < x1; x++) {
						const halnf alpha = mSelection.coverage(x, y) / 255.0f * std::clamp(color.a, 0.0f, 1.0f);
						if (alpha <= 0) continue;
						if (!tile) tile = mActive->tileAt(coord);
						uint1* pixel =
							tile->pixels() + ((size_t) (y - ty * kTileSize) * kTileSize + (size_t) (x - tx * kTileSize)) * 4;
						if (keepAlpha) {
							const halnf existing = pixel[3] / 255.0f;
							for (int4 channel = 0; channel < 3; channel++) {
								const halnf current = existing > 0 ? pixel[channel] / 255.0f / existing : 0;
								pixel[channel] = (uint1) std::lround((current + (rgb[channel] - current) * alpha) * existing * 255.0f);
							}
							continue;
						}
						for (int4 channel = 0; channel < 3; channel++)
							pixel[channel] = (uint1) std::lround(rgb[channel] * alpha * 255.0f + pixel[channel] * (1 - alpha));
						pixel[3] = (uint1) std::lround(alpha * 255.0f + pixel[3] * (1 - alpha));
					}
				if (tile)
					tile->markDirty(
						{ (halnf) (x0 - tx * kTileSize), (halnf) (y0 - ty * kTileSize), (halnf) (x1 - x0), (halnf) (y1 - y0) }
					);
			}
		return SelectionEditResult::Done;
	}

	Document::SelectionEditResult Document::fillSelectionUnderneath(const RGBA& color) {
		if (!mSelection.isActive()) return SelectionEditResult::NoSelection;
		if (mSelection.isInverted()) return SelectionEditResult::Inverted;
		if (!mActive || !canAddLayer()) return SelectionEditResult::Locked;
		Layer* above = mActive;
		Layer* fill = addLayer("Fill");
		if (!fill) return SelectionEditResult::Locked;
		size_t aboveIndex = 0;
		for (size_t index = 0; index < mLayers.size(); index++)
			if (mLayers[index].get() == above) aboveIndex = index;
		size_t fillIndex = mLayers.size() - 1;
		for (size_t index = 0; index < mLayers.size(); index++)
			if (mLayers[index].get() == fill) fillIndex = index;
		std::unique_ptr<Layer> moved = std::move(mLayers[fillIndex]);
		mLayers.erase(mLayers.begin() + (std::ptrdiff_t) fillIndex);
		if (fillIndex < aboveIndex) aboveIndex--;
		mLayers.insert(mLayers.begin() + (std::ptrdiff_t) aboveIndex, std::move(moved));
		mActive = fill;
		const SelectionEditResult result = fillSelection(color);
		mActive = above;
		return result;
	}

	Document::SelectionEditResult Document::clearSelection() {
		settlePaint();
		if (!mActive || !canEdit(*mActive)) return SelectionEditResult::Locked;
		if (mActive->alphaLocked()) return SelectionEditResult::AlphaLocked;
		if (!mSelection.isActive()) {
			mActive->clear();
			return SelectionEditResult::Done;
		}
		std::vector<TileCoord> coords;
		mActive->forEachTile([&coords](const TileCoord& coord, Tile*) { coords.push_back(coord); });
		for (const TileCoord& coord : coords) {
			Tile* tile = nullptr;
			for (int4 y = 0; y < kTileSize; y++)
				for (int4 x = 0; x < kTileSize; x++) {
					const halnf cut = mSelection.coverage(coord.x * kTileSize + x, coord.y * kTileSize + y) / 255.0f;
					if (cut <= 0) continue;
					if (!tile) tile = mActive->tileAt(coord);
					uint1* pixel = tile->pixels() + ((size_t) y * kTileSize + (size_t) x) * 4;
					for (int4 channel = 0; channel < 4; channel++)
						pixel[channel] = (uint1) std::lround(pixel[channel] * (1 - cut));
				}
			if (tile) {
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
				if (tile->isTransparent()) mActive->removeTile(coord);
			}
		}
		return SelectionEditResult::Done;
	}

	bool Document::deselect() {
		if (!mSelection.isActive()) return false;
		mSelection.clear();
		return true;
	}

	bool Document::selectAllCanvas() {
		int4 left = 0, top = 0, right = 0, bottom = 0;
		if (!mFrame.infinite && !mFrame.isEmpty()) {
			left = mFrame.left;
			top = mFrame.top;
			right = mFrame.right;
			bottom = mFrame.bottom;
		} else if (!contentBounds(left, top, right, bottom)) {
			return false;
		}
		mSelection.selectAll(left, top, right, bottom);
		return true;
	}

	bool Document::selectAllAsRectangle() {
		settlePaint();
		int4 left = 0, top = 0, right = 0, bottom = 0;
		if (!mActive || !layerBounds(*mActive, left, top, right, bottom)) return false;
		mSelection.selectAll(left, top, right, bottom);
		return true;
	}

	bool Document::selectAllAsLasso() {
		settlePaint();
		int4 left = 0, top = 0, right = 0, bottom = 0;
		if (!mActive || !layerBounds(*mActive, left, top, right, bottom)) return false;
		const int4 width = right - left;
		std::vector<uint1> mask((size_t) width * (bottom - top), 0);
		mActive->forEachTile([&](const TileCoord& coord, Tile* tile) {
			for (int4 y = 0; y < kTileSize; y++)
				for (int4 x = 0; x < kTileSize; x++) {
					const int4 px = coord.x * kTileSize + x;
					const int4 py = coord.y * kTileSize + y;
					if (px < left || px >= right || py < top || py >= bottom) continue;
					mask[(size_t) (py - top) * width + (px - left)] = tile->pixels()[((ualni) y * kTileSize + (ualni) x) * 4 + 3];
				}
		});
		mSelection.clear();
		mSelection.addMask(left, top, width, mask, SelectionMode::Replace);
		return true;
	}

	halnf Document::pixelValue(const halnf rgba[4]) { return 0.299f * rgba[0] + 0.587f * rgba[1] + 0.114f * rgba[2]; }

	namespace {
		class PixelFunctionOperation : public Operation {
		public:
			PixelFunctionOperation(std::function<void(halnf rgba[4])> function, std::shared_ptr<const Selection> selection) :
				mFunction(std::move(function)),
				mSelection(std::move(selection)) {}

			[[nodiscard]] const char* name() const override { return "pixel function"; }

			void prepare(const TileCoord& coord, std::shared_ptr<const Tile> source) {
				mSources.put(coord.key(), std::move(source));
			}

			void run(const TileWork& work, Tile& target) override {
				const Tile& source = *mSources.get(work.coord.key());
				const TileCoord& coord = work.coord;
				for (int4 y = 0; y < kTileSize; y++)
					for (int4 x = 0; x < kTileSize; x++) {
						const size_t offset = ((size_t) y * kTileSize + (size_t) x) * 4;
						const uint1* current = source.pixels() + offset;
						uint1* pixel = target.pixels() + offset;
						const halnf amount =
							mSelection ? mSelection->coverage(coord.x * kTileSize + x, coord.y * kTileSize + y) / 255.0f : 1.0f;
						if (amount <= 0 || current[3] == 0) {
							std::copy(current, current + 4, pixel);
							continue;
						}
						halnf before[4];
						before[3] = current[3] / 255.0f;
						for (int4 channel = 0; channel < 3; channel++)
							before[channel] = current[channel] / 255.0f / before[3];
						halnf after[4] = { before[0], before[1], before[2], before[3] };
						mFunction(after);
						const halnf alpha = std::clamp(before[3] + (after[3] - before[3]) * amount, 0.0f, 1.0f);
						for (int4 channel = 0; channel < 3; channel++) {
							const halnf value = std::clamp(before[channel] + (after[channel] - before[channel]) * amount, 0.0f, 1.0f);
							pixel[channel] = (uint1) std::lround(value * alpha * 255.0f);
						}
						pixel[3] = (uint1) std::lround(alpha * 255.0f);
					}
			}

			void finish(Layer& layer, const TileWork& work, const std::shared_ptr<Tile>& result) override {
				if (layer.sharedTile(work.coord).get() != mSources.get(work.coord.key()).get()) return;
				if (result->isTransparent()) {
					layer.removeTile(work.coord);
					return;
				}
				layer.replaceTile(work.coord, result);
				Operation::finish(layer, work, result);
			}

		private:
			std::function<void(halnf rgba[4])> mFunction;
			std::shared_ptr<const Selection> mSelection;
			Map<ualni, std::shared_ptr<const Tile>> mSources;
		};
	}

	Document::FilterResult
	Document::applyPixelFunction(const std::function<void(halnf rgba[4])>& function, bool changesAlpha) {
		settlePaint();
		if (!mActive || !canEdit(*mActive)) return FilterResult::Locked;
		if (changesAlpha && mActive->alphaLocked()) return FilterResult::AlphaLocked;
		if (mActive->tileCount() == 0) return FilterResult::Empty;
		auto operation = std::make_shared<PixelFunctionOperation>(
			function, mSelection.isActive() ? std::make_shared<const Selection>(mSelection) : nullptr
		);
		const Layer::State before = mActive->state();
		for (auto it : before.tiles) {
			const TileCoord coord = it->val.coord;
			operation->prepare(coord, it->val.tile);
			mEngine.submit({ operation, nullptr, mActive, { coord, 0 } });
		}
		return FilterResult::Done;
	}

	Document::FilterResult Document::applyFilter(PixelFilter filter) {
		switch (filter) {
			case PixelFilter::Invert:
				return applyPixelFunction(
					[](halnf rgba[4]) {
						for (int4 channel = 0; channel < 3; channel++)
							rgba[channel] = 1.0f - rgba[channel];
					},
					false
				);
			case PixelFilter::Desaturate:
				return applyPixelFunction(
					[](halnf rgba[4]) {
						const halnf value = pixelValue(rgba);
						rgba[0] = rgba[1] = rgba[2] = value;
					},
					false
				);
			case PixelFilter::ValueToAlpha:
				return applyPixelFunction(
					[](halnf rgba[4]) {
						rgba[3] *= 1.0f - pixelValue(rgba);
						rgba[0] = rgba[1] = rgba[2] = 0;
					},
					true
				);
			case PixelFilter::ValueToAlphaInverted:
				return applyPixelFunction(
					[](halnf rgba[4]) {
						rgba[3] *= pixelValue(rgba);
						rgba[0] = rgba[1] = rgba[2] = 1;
					},
					true
				);
		}
		return FilterResult::Empty;
	}

	Document::PixelClip Document::copyPixels() const {
		settlePaint();
		PixelClip clip;
		if (!mActive) return clip;
		int4 left = 0, top = 0, right = 0, bottom = 0;
		if (!layerBounds(*mActive, left, top, right, bottom)) return clip;
		if (mSelection.isActive() && !mSelection.isInverted()) {
			int4 sl = 0, st = 0, sr = 0, sb = 0;
			if (!mSelection.bounds(sl, st, sr, sb)) return clip;
			left = std::max(left, sl);
			top = std::max(top, st);
			right = std::min(right, sr);
			bottom = std::min(bottom, sb);
			if (right <= left || bottom <= top) return clip;
		}
		clip.left = left;
		clip.top = top;
		clip.width = right - left;
		clip.height = bottom - top;
		clip.pixels.assign((size_t) clip.width * clip.height * 4, 0);
		bool any = false;
		for (int4 y = top; y < bottom; y++)
			for (int4 x = left; x < right; x++) {
				const TileCoord coord = { floordiv(x, kTileSize), floordiv(y, kTileSize) };
				const Tile* tile = mActive->tileIfExists(coord);
				if (!tile) continue;
				const uint1* pixel =
					tile->pixels() + ((size_t) (y - coord.y * kTileSize) * kTileSize + (size_t) (x - coord.x * kTileSize)) * 4;
				const uint4 take = mSelection.isActive() ? mSelection.coverage(x, y) : 255;
				uint1* target = clip.pixels.data() + ((size_t) (y - top) * clip.width + (size_t) (x - left)) * 4;
				for (int4 channel = 0; channel < 4; channel++)
					target[channel] = (uint1) ((pixel[channel] * take + 127) / 255);
				any = any || target[3] != 0;
			}
		if (!any) return {};
		return clip;
	}

	Layer* Document::pasteAsLayer(const PixelClip& clip, const std::string& name) {
		settlePaint();
		if (clip.isEmpty()) return nullptr;
		Layer* layer = addLayer(name);
		if (!layer) return nullptr;
		for (int4 y = 0; y < clip.height; y++)
			for (int4 x = 0; x < clip.width; x++) {
				const uint1* source = clip.pixels.data() + ((size_t) y * clip.width + (size_t) x) * 4;
				if (source[3] == 0) continue;
				const int4 px = clip.left + x;
				const int4 py = clip.top + y;
				const TileCoord coord = { floordiv(px, kTileSize), floordiv(py, kTileSize) };
				Tile* tile = layer->tileAt(coord);
				uint1* target =
					tile->pixels() + ((size_t) (py - coord.y * kTileSize) * kTileSize + (size_t) (px - coord.x * kTileSize)) * 4;
				std::copy(source, source + 4, target);
				tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
			}
		return layer;
	}

	namespace {
		void rgbToHsl(const halnf rgb[3], halnf& h, halnf& s, halnf& l) {
			const halnf high = std::max({ rgb[0], rgb[1], rgb[2] });
			const halnf low = std::min({ rgb[0], rgb[1], rgb[2] });
			l = (high + low) / 2;
			const halnf delta = high - low;
			if (delta <= 1e-6f) {
				h = s = 0;
				return;
			}
			s = l > 0.5f ? delta / (2 - high - low) : delta / (high + low);
			if (high == rgb[0]) h = (rgb[1] - rgb[2]) / delta + (rgb[1] < rgb[2] ? 6.0f : 0.0f);
			else if (high == rgb[1]) h = (rgb[2] - rgb[0]) / delta + 2;
			else h = (rgb[0] - rgb[1]) / delta + 4;
			h /= 6;
		}

		halnf hueChannel(halnf p, halnf q, halnf t) {
			if (t < 0) t += 1;
			if (t > 1) t -= 1;
			if (t < 1.0f / 6) return p + (q - p) * 6 * t;
			if (t < 0.5f) return q;
			if (t < 2.0f / 3) return p + (q - p) * (2.0f / 3 - t) * 6;
			return p;
		}

		void hslToRgb(halnf h, halnf s, halnf l, halnf rgb[3]) {
			if (s <= 0) {
				rgb[0] = rgb[1] = rgb[2] = l;
				return;
			}
			const halnf q = l < 0.5f ? l * (1 + s) : l + s - l * s;
			const halnf p = 2 * l - q;
			rgb[0] = hueChannel(p, q, h + 1.0f / 3);
			rgb[1] = hueChannel(p, q, h);
			rgb[2] = hueChannel(p, q, h - 1.0f / 3);
		}

		void applyLightness(halnf rgb[3], halnf lightness) {
			for (int4 channel = 0; channel < 3; channel++)
				rgb[channel] = lightness >= 0 ? rgb[channel] + (1 - rgb[channel]) * lightness : rgb[channel] * (1 + lightness);
		}
	}

	bool Adjustment::isNeutral() const {
		switch (kind) {
			case AdjustmentKind::BrightnessExposure: return brightness == 0 && exposure == 0;
			case AdjustmentKind::HueSaturation: return hueDegrees == 0 && saturation == 0 && lightness == 0;
			case AdjustmentKind::ColorBalance: return red == 0 && green == 0 && blue == 0;
			case AdjustmentKind::Colorize: return false;
			case AdjustmentKind::Alpha: return alpha == 0;
		}
		return true;
	}

	void Adjustment::apply(halnf rgba[4]) const {
		switch (kind) {
			case AdjustmentKind::BrightnessExposure:
				{
					const halnf gain = std::pow(2.0f, exposure);
					for (int4 channel = 0; channel < 3; channel++)
						rgba[channel] = rgba[channel] * gain + brightness;
					break;
				}
			case AdjustmentKind::HueSaturation:
				{
					halnf h = 0, s = 0, l = 0;
					rgbToHsl(rgba, h, s, l);
					h = std::fmod(h + hueDegrees / 360.0f + 1.0f, 1.0f);
					s = std::clamp(saturation >= 0 ? s + (1 - s) * saturation : s * (1 + saturation), 0.0f, 1.0f);
					hslToRgb(h, s, l, rgba);
					applyLightness(rgba, lightness);
					break;
				}
			case AdjustmentKind::ColorBalance:
				{
					const halnf shift[3] = { red, green, blue };
					for (int4 channel = 0; channel < 3; channel++)
						rgba[channel] = shift[channel] >= 0 ? rgba[channel] + (1 - rgba[channel]) * shift[channel]
																								: rgba[channel] * (1 + shift[channel]);
					break;
				}
			case AdjustmentKind::Colorize:
				{
					halnf h = 0, s = 0, l = 0;
					rgbToHsl(rgba, h, s, l);
					hslToRgb(std::fmod(hueDegrees / 360.0f + 1.0f, 1.0f), std::clamp(saturation, 0.0f, 1.0f), l, rgba);
					applyLightness(rgba, lightness);
					break;
				}
			case AdjustmentKind::Alpha: rgba[3] = alpha >= 0 ? rgba[3] + (1 - rgba[3]) * alpha : rgba[3] * (1 + alpha); break;
		}
	}

	bool Document::beginAdjustment() {
		settlePaint();
		if (!mActive || !canEdit(*mActive) || mActive->tileCount() == 0) return false;
		mAdjustSource = mActive->state();
		mAdjustChanged = false;
		return true;
	}

	void Document::previewAdjustment(const Adjustment& adjustment) {
		if (!mAdjustSource || !mActive) return;
		settlePaint();
		mActive->restore(*mAdjustSource);
		mAdjustChanged = !adjustment.isNeutral();
		if (!mAdjustChanged) return;
		const bool changesAlpha = adjustment.kind == AdjustmentKind::Alpha;
		if (changesAlpha && mActive->alphaLocked()) return;
		applyPixelFunction([adjustment](halnf rgba[4]) { adjustment.apply(rgba); }, changesAlpha);
	}

	bool Document::finishAdjustment() {
		if (!mAdjustSource) return false;
		const bool changed = mAdjustChanged;
		if (!changed && mActive) mActive->restore(*mAdjustSource);
		mAdjustSource.reset();
		return changed;
	}

	void Document::cancelAdjustment() {
		if (!mAdjustSource) return;
		if (mActive) mActive->restore(*mAdjustSource);
		mAdjustSource.reset();
	}

	bool Document::invertSelection() {
		if (!mSelection.isActive()) return false;
		mSelection.invert();
		return true;
	}

	bool Document::contentBounds(int4& left, int4& top, int4& right, int4& bottom) const {
		settlePaint();
		bool any = false;
		forEachTile([&](const TileCoord& coord, Tile* tile) {
			for (int4 y = 0; y < kTileSize; y++) {
				for (int4 x = 0; x < kTileSize; x++) {
					if (tile->pixels()[((ualni) y * kTileSize + (ualni) x) * 4 + 3] == 0) continue;
					const int4 px = coord.x * kTileSize + x;
					const int4 py = coord.y * kTileSize + y;
					if (!any) {
						left = right = px;
						top = bottom = py;
						any = true;
					}
					left = std::min(left, px);
					top = std::min(top, py);
					right = std::max(right, px + 1);
					bottom = std::max(bottom, py + 1);
				}
			}
		});
		return any;
	}

	CanvasFrame Document::proposedFrame(Vec2F viewCenterWorld) const {
		CanvasFrame frame = mFrame;
		frame.infinite = false;

		if (frame.isEmpty()) {
			int4 left = 0;
			int4 top = 0;
			int4 right = 0;
			int4 bottom = 0;
			if (contentBounds(left, top, right, bottom)) {
				frame.left = left;
				frame.top = top;
				frame.right = right;
				frame.bottom = bottom;
			} else {
				const int4 cx = (int4) std::lround(viewCenterWorld.x);
				const int4 cy = (int4) std::lround(viewCenterWorld.y);
				frame.left = cx - CanvasFrame::kDefaultHalfSize;
				frame.top = cy - CanvasFrame::kDefaultHalfSize;
				frame.right = cx + CanvasFrame::kDefaultHalfSize;
				frame.bottom = cy + CanvasFrame::kDefaultHalfSize;
			}
		}

		if (frame.width() > CanvasFrame::kMaximumSide) {
			const int4 center = (frame.left + frame.right) / 2;
			frame.left = center - CanvasFrame::kMaximumSide / 2;
			frame.right = center + CanvasFrame::kMaximumSide / 2;
		}
		if (frame.height() > CanvasFrame::kMaximumSide) {
			const int4 center = (frame.top + frame.bottom) / 2;
			frame.top = center - CanvasFrame::kMaximumSide / 2;
			frame.bottom = center + CanvasFrame::kMaximumSide / 2;
		}
		frame.clamp();
		return frame;
	}

	bool Document::setInfinite(bool infinite, Vec2F viewCenterWorld) {
		if (infinite == mFrame.infinite) return false;
		if (infinite) mFrame.infinite = true;
		else mFrame = proposedFrame(viewCenterWorld);
		return true;
	}

	bool Document::setFrame(int4 left, int4 top, int4 right, int4 bottom) {
		CanvasFrame frame = mFrame;
		frame.left = left;
		frame.top = top;
		frame.right = right;
		frame.bottom = bottom;
		frame.clamp();
		if (frame == mFrame) return false;
		mFrame = frame;
		return true;
	}

	bool Document::setBackground(const CanvasBackground& background) {
		if (background == mBackground) return false;
		mBackground = background;
		return true;
	}

	void Document::restore(const DocumentState& snapshot) {
		settlePaint();
		mBackground = snapshot.background;
		mFrame = snapshot.frame;
		mSelection = snapshot.selection;
		std::vector<std::unique_ptr<Layer>> layers;
		layers.reserve(snapshot.layers.size());

		for (const Layer::State& layerState : snapshot.layers) {
			std::unique_ptr<Layer> layer;
			for (auto& existing : mLayers) {
				if (existing && existing->id() == layerState.id) layer = std::move(existing);
			}
			if (!layer) layer = std::make_unique<Layer>(layerState.id, layerState.properties.name);
			layer->restore(layerState);
			layers.push_back(std::move(layer));
		}

		mLayers = std::move(layers);
		mActive = layerWithId(snapshot.active);
		if (!mActive && !mLayers.empty()) mActive = mLayers.back().get();
		mStrokeLayer = nullptr;
		mStrokeTiles.removeAll();
		mStrokeMasks.removeAll();
		mPreviewMasks.removeAll();
		mSmudgeCarry.clear();
	}

	void Document::load(const DocumentState& snapshot, const std::string& label) {
		restore(snapshot);
		for (const auto& layer : mLayers)
			mNextLayerId = std::max(mNextLayerId, layer->id() + 1);
		mHistory.reset(label, state());
	}

	void Document::commit(const std::string& label, const std::string& mergeKey) {
		mHistory.commit(label, state(), mergeKey);
	}

	void Document::restoreStrokeBase() {
		mSmudgeCarry.clear();
		if (mPreviewLevel >= 0) {
			Layer* layer = mStrokeLayer;
			dropOpenStroke();
			mStrokeLayer = layer;
			if (!layer) return;
			for (const Layer::State& layerState : mHistory.current().state.layers)
				if (layerState.id == layer->id()) layer->restore(layerState);
			return;
		}
		settlePaint();
		mStrokeMasks.removeAll();
		mPreviewMasks.removeAll();
		if (!mStrokeLayer) return;
		for (const Layer::State& layerState : mHistory.current().state.layers)
			if (layerState.id == mStrokeLayer->id()) mStrokeLayer->restore(layerState);
	}

	void Document::discardStroke() {
		settlePaint();
		mStrokeMasks.removeAll();
		mPreviewMasks.removeAll();
		mSmudgeCarry.clear();
		if (!mStrokeLayer) return;
		restore(mHistory.current().state);
	}

	void Document::dropOpenStroke() {
		resetPathStroke();
		const bool open = mOpenTrace && !mOpenTrace->isEmpty();
		if (mPreviewLevel < 0 && !open) return;
		if (mStrokeCancelled) mStrokeCancelled->store(true);
		mStrokeCancelled.reset();
		mPendingSlots.removeAll();
		mOpenTrace.reset();
		mPreviewMasks.removeAll();
		mStrokeCoverage.removeAll();
		mPreviewLevel = -1;
		mLastPreviewDab.reset();
		mStrokeLayer = nullptr;
		mPendingStrokeEntries.clear();
		for (const auto& layer : mLayers)
			if (layer->levels().previewCount() > 0) {
				layer->levels().forEachBasePreview([&](const TileCoord& coord, Tile*) { layer->noteDisplayChanged(coord); });
				layer->levels().discardPreview();
			}
	}

	bool Document::undo() {
		dropOpenStroke();
		settlePaint();
		const DocumentState* snapshot = mHistory.undo();
		if (snapshot) restore(*snapshot);
		return snapshot != nullptr;
	}

	bool Document::redo() {
		dropOpenStroke();
		settlePaint();
		const DocumentState* snapshot = mHistory.redo();
		if (snapshot) restore(*snapshot);
		return snapshot != nullptr;
	}

	bool Document::jumpToHistory(size_t index) {
		dropOpenStroke();
		settlePaint();
		const DocumentState* snapshot = mHistory.jumpTo(index);
		if (snapshot) restore(*snapshot);
		return snapshot != nullptr;
	}

	Layer* Document::layerWithId(ualni id) const {
		for (const auto& layer : mLayers) {
			if (layer->id() == id) return layer.get();
		}
		return nullptr;
	}

	namespace {
		constexpr halnf kFullTurn = 6.28318530718f;

		Vec2F rotated(const Vec2F& vector, halnf radians) {
			const halnf cosine = std::cos(radians);
			const halnf sine = std::sin(radians);
			return { vector.x * cosine - vector.y * sine, vector.x * sine + vector.y * cosine };
		}

		halnf normalizedAngle(halnf radians) {
			halnf angle = std::fmod(radians, kFullTurn);
			if (angle > kFullTurn / 2) angle -= kFullTurn;
			if (angle <= -kFullTurn / 2) angle += kFullTurn;
			if (std::abs(angle) < 1.0e-6f) angle = 0;
			return angle;
		}
	}

	void Document::pan(const Vec2F& screenDelta) { mPan -= rotated(screenDelta, -mRotation) / mZoom; }

	void Document::setRotation(halnf radians) { mRotation = normalizedAngle(radians); }

	void Document::rotateAt(const Vec2F& screenPoint, halnf radians) { setRotationAt(screenPoint, mRotation + radians); }

	void Document::setRotationAt(const Vec2F& screenPoint, halnf radians) {
		const Vec2F worldBefore = screenToWorld(screenPoint);
		setRotation(radians);
		const Vec2F worldAfter = screenToWorld(screenPoint);
		mPan += worldBefore - worldAfter;
	}

	void Document::zoomAt(const Vec2F& screenPoint, halnf factor) {
		const Vec2F worldBefore = screenToWorld(screenPoint);
		mZoom *= factor;
		if (mZoom < 0.02f) mZoom = 0.02f;
		if (mZoom > 64.0f) mZoom = 64.0f;
		const Vec2F worldAfter = screenToWorld(screenPoint);
		mPan += worldBefore - worldAfter;
	}

	void Document::resetView() {
		mPan = { 0, 0 };
		mZoom = 1.0f;
		mRotation = 0;
	}

	Vec2F Document::screenToWorld(const Vec2F& screen) const { return mPan + rotated(screen, -mRotation) / mZoom; }

	Vec2F Document::worldToScreen(const Vec2F& world) const { return rotated(worldToView(world), mRotation); }

	Vec2F Document::screenToView(const Vec2F& screen) const { return rotated(screen, -mRotation); }

	RectF Document::viewBoundsOfScreen(Vec2F screenSize) const {
		const Vec2F corners[] = { screenToView({ 0, 0 }),
															screenToView(Vec2F(screenSize.x, 0.0f)),
															screenToView(Vec2F(0.0f, screenSize.y)),
															screenToView(screenSize) };
		Vec2F low = corners[0];
		Vec2F high = corners[0];
		for (const Vec2F& corner : corners) {
			low = Vec2F(std::min(low.x, corner.x), std::min(low.y, corner.y));
			high = Vec2F(std::max(high.x, corner.x), std::max(high.y, corner.y));
		}
		return { low.x, low.y, high.x - low.x, high.y - low.y };
	}

	ualni Document::tileCount() const {
		settlePaint();
		ualni total = 0;
		for (const auto& layer : mLayers)
			total += layer->tileCount();
		return total;
	}

	void Document::finishStroke(const std::string& label) {
		TP_PROFILE_SCOPE("brush.finish_stroke");
		advancePathStroke(true);
		const bool deferred = mPathRangeOpen || (mOpenTrace && !mOpenTrace->isEmpty()) || mPreviewLevel >= 0;
		flushDeferredDabs();
		resetPathStroke();
		mPainter.flush();
		mSmudgeCarry.clear();
		if (!mStrokeLayer) return;
		if (deferred) {
			mStrokeTiles.removeAll();
			mStrokeMasks.removeAll();
			mPreviewMasks.removeAll();
			mStrokeCoverage.removeAll();
			mStrokeCancelled.reset();
			mPreviewLevel = -1;
			mLastPreviewDab.reset();
			mStrokeLayer = nullptr;
			TP_PROFILE_SCOPE("history.commit");
			mHistory.commit(label, stateWithoutSettling(), {});
			for (const std::shared_ptr<ualni>& entry : mPendingStrokeEntries)
				if (*entry == 0) *entry = mHistory.currentEntryRevision();
			mPendingStrokeEntries.clear();
			return;
		}
		mEngine.drain();

		if (mStrokeLayer == mActive) {
			for (auto it : mStrokeTiles) {
				const TileCoord coord = it->val;
				Tile* tile = mActive->tileIfExists(coord);
				if (tile && tile->isTransparent()) mActive->removeTile(coord);
			}
		}
		mStrokeTiles.removeAll();
		mStrokeMasks.removeAll();
		mPreviewMasks.removeAll();
		mStrokeCoverage.removeAll();
		mStrokeCancelled.reset();
		mPreviewLevel = -1;
		mLastPreviewDab.reset();
		mStrokeLayer = nullptr;
		TP_PROFILE_SCOPE("history.commit");
		commit(label);
	}

	bool Document::isLayerEmpty(const Layer& layer) const {
		return layer.tileCount() == 0 && layer.levels().previewCount() == 0 && !mEngine.hasWorkFor(layer);
	}

	std::string Document::strokeRefusal(const Brush& brush) const {
		if (!mActive) return "Please select a layer";
		if (mActive->locked()) return "You can't edit a locked layer";

		const bool erase = brush.erases();
		if (erase && mActive->alphaLocked()) return "You can't erase on a alpha locked layer";
		if (erase && isLayerEmpty(*mActive)) return "You can't erase a empty layer";
		if (brush.smudges() && isLayerEmpty(*mActive)) return "You can't smudge a empty layer";
		return {};
	}

	void Document::stampBrushDab(const Vec2F& worldCenter, const Brush& brush, halnf pressure) {
		TP_PROFILE_SCOPE("brush.dab");
		TP_PROFILE_COUNT("brush.dabs", 1);
		if (brush.smudges()) {
			smudgeDab(worldCenter, brush, brush.dabAt(pressure));
			return;
		}
		stampDab(worldCenter, brush, brush.dabAt(pressure));
	}

	void Document::smudgeDab(const Vec2F& worldCenter, const Brush& brush, const DabShape& dab) {
		settlePaint();
		TP_PROFILE_SCOPE("brush.smudge_dab");
		if (!mActive || !canEdit(*mActive)) return;
		if (dab.radius() <= 0.0f) return;

		const int4 diameter = (int4) std::ceil(dab.size) + 1;
		const int4 originX = (int4) std::floor(worldCenter.x - diameter * 0.5f);
		const int4 originY = (int4) std::floor(worldCenter.y - diameter * 0.5f);
		const bool linear = brush.linearMixing();

		SmudgePatch canvas = readSmudgePatch(*mActive, originX, originY, diameter, linear);
		const SmudgePatch pickup = blurredSmudgePatch(canvas, smudgeBlurRadius(brush.get(BrushParameter::Blur)));

		if (mSmudgeCarry.isEmpty()) {
			mSmudgeCarry.diameter = diameter;
			mSmudgeCarry.rgba = pickup.rgba;
		} else if (mSmudgeCarry.diameter != diameter) {
			resampleSmudgeCarry(mSmudgeCarry, diameter);
		}

		const halnf strength = dab.flow * dab.opacity;
		const bool keepAlpha = mActive->alphaLocked();
		for (int4 y = 0; y < diameter; y++) {
			for (int4 x = 0; x < diameter; x++) {
				const halnf wx = (halnf) (originX + x) + 0.5f - worldCenter.x;
				const halnf wy = (halnf) (originY + y) + 0.5f - worldCenter.y;
				halnf distance = 0;
				halnf a = dab.coverage(wx, wy, &distance);
				if (a <= 0.0f) continue;
				a = dab.textured(a, { (halnf) (originX + x), (halnf) (originY + y) }, distance) * strength;
				if (a <= 0.0f) continue;

				if (mSelection.isActive()) {
					a *= mSelection.coverage(originX + x, originY + y) / 255.0f;
					if (a <= 0.0f) continue;
				}
				halnf* pixel = canvas.at(x, y);
				const halnf* carried = mSmudgeCarry.rgba.data() + ((size_t) y * diameter + x) * 4;
				const halnf previousAlpha = pixel[3];
				for (int4 channel = 0; channel < 4; channel++)
					pixel[channel] += (carried[channel] - pixel[channel]) * a;
				if (keepAlpha) keepSmudgedAlpha(pixel, previousAlpha);
			}
		}
		writeSmudgePatch(canvas, originX, originY, keepAlpha, linear);

		const halnf keep = brush.get(BrushParameter::Smudge);
		for (size_t index = 0; index < mSmudgeCarry.rgba.size(); index++) {
			halnf& carried = mSmudgeCarry.rgba[index];
			carried = pickup.rgba[index] + (carried - pickup.rgba[index]) * keep;
		}
		resaturateSmudgeCarry(mSmudgeCarry, brush, brush.get(BrushParameter::Resaturation));
	}

	void Document::writeSmudgePatch(const SmudgePatch& patch, int4 originX, int4 originY, bool keepAlpha, bool linear) {
		forEachTileSpan(originX, originY, patch.diameter, [&](TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1) {
			if (keepAlpha && !mActive->tileIfExists(coord)) return;
			Tile* tile = mActive->tileAt(coord);
			mStrokeLayer = mActive;
			mStrokeTiles.put(coord.key(), coord);

			for (int4 y = y0; y < y1; y++) {
				for (int4 x = x0; x < x1; x++) {
					const int4 lx = x - coord.x * kTileSize;
					const int4 ly = y - coord.y * kTileSize;
					uint1* target = tile->pixels() + ((ualni) ly * kTileSize + lx) * 4;
					encodeMixPixel(patch.at(x - originX, y - originY), target, linear);
				}
			}
			const int4 left = x0 - coord.x * kTileSize;
			const int4 top = y0 - coord.y * kTileSize;
			tile->markDirty({ (halnf) left, (halnf) top, (halnf) (x1 - x0), (halnf) (y1 - y0) });
		});
	}

	StrokeMask* Document::strokeMask(const TileCoord& coord) {
		auto found = mStrokeMasks.presents(coord.key());
		if (found.isValid()) return mStrokeMasks.getSlotVal(found).get();
		auto mask = std::make_shared<StrokeMask>();
		mask->setBase(mActive->sharedTile(coord));
		StrokeMask* raw = mask.get();
		mStrokeMasks.put(coord.key(), std::move(mask));
		return raw;
	}

	void Document::stampDab(const Vec2F& worldCenter, const Brush& brush, const DabShape& requested) {
		TP_PROFILE_SCOPE("brush.stamp");
		if (!mActive || !canEdit(*mActive)) return;

		const std::optional<TracedDab> traced = traceDab(brush, requested, strokeTarget(), worldCenter);
		if (!traced) return;

		const int4 level = deferredLevel(traced->work->marker);
		if (level >= 0) {
			usePreviewLevel(level);
			openTrace().record(*traced);
			stagePreviewDab(*traced);
			return;
		}
		if (mPreviewLevel >= 0) settlePaint();
		stageEagerDab(*traced);
	}

	void Document::stageEagerDab(const TracedDab& dab) {
		const bool masked = dab.masked();
		for (int4 ty = floordiv(dab.minY, kTileSize); ty <= floordiv(dab.maxY, kTileSize); ty++) {
			for (int4 tx = floordiv(dab.minX, kTileSize); tx <= floordiv(dab.maxX, kTileSize); tx++) {
				const TileCoord coord = { tx, ty };
				const int4 originX = tx * kTileSize;
				const int4 originY = ty * kTileSize;
				const int4 lx0 = std::max(0, dab.minX - originX);
				const int4 ly0 = std::max(0, dab.minY - originY);
				const int4 lx1 = std::min(kTileSize, dab.maxX - originX);
				const int4 ly1 = std::min(kTileSize, dab.maxY - originY);
				if (lx1 <= lx0 || ly1 <= ly0) continue;
				if (dab.work->keepAlpha && !mActive->tileIfExists(coord)) continue;

				StrokeMask* mask = masked ? strokeMask(coord) : nullptr;
				Tile* tile = mActive->tileAt(coord);
				mStrokeLayer = mActive;
				mStrokeTiles.put(coord.key(), coord);
				mPainter.stage(dab.work, *tile, *mActive, coord, lx0, ly0, lx1, ly1, mask);
				TP_PROFILE_COUNT("brush.pixels_touched", (lx1 - lx0) * (ly1 - ly0));
			}
		}
	}

	DabTarget Document::strokeTarget() {
		DabTarget target;
		target.keepAlpha = mActive->alphaLocked();
		if (mSelection.isActive()) {
			if (!mStrokeSelection || !(*mStrokeSelection == mSelection))
				mStrokeSelection = std::make_shared<const Selection>(mSelection);
			target.selection = mStrokeSelection;
		}
		return target;
	}

	int4 Document::deferredLevel(bool marker) const {
		const int4 level = LayerLevels::levelForZoom(mZoom);
		if (marker) return -1;
		if (level > 0 && mActive->levels().builtLevels() < level) return -1;
		return level;
	}

	void Document::usePreviewLevel(int4 level) {
		mStrokeLayer = mActive;
		if (mPreviewLevel == level) return;
		flushDeferredDabs();
		mPreviewMasks.removeAll();
		mLastPreviewDab.reset();
		mPreviewLevel = level;
	}

	StrokeTrace& Document::openTrace() {
		if (!mOpenTrace) mOpenTrace = std::make_shared<StrokeTrace>();
		return *mOpenTrace;
	}

	void Document::beginPathStroke(const Brush& brush, StrokeMirror mirror, const StrokePath::Sample& first) {
		resetPathStroke();
		if (!mActive || !canEdit(*mActive)) return;
		mPathStyle = { brush, strokeTarget(), std::move(mirror) };
		mStrokePath.begin(first);
		mPathActive = true;
		if (brush.smudges()) {
			stampBrushDab(first.position, brush, first.pressure);
			return;
		}
		mPathStyle.forEachDab(first.position, first.pressure, [this](TracedDab dab) {
			TP_PROFILE_COUNT("brush.dabs", 1);
			const int4 level = deferredLevel(dab.work->marker);
			if (level < 0) {
				if (mPreviewLevel >= 0) settlePaint();
				stageEagerDab(dab);
				return;
			}
			usePreviewLevel(level);
			openTrace().record(dab);
			stagePreviewDab(dab);
		});
	}

	void Document::addPathSample(const StrokePath::Sample& sample) {
		if (!mPathActive) return;
		mStrokePath.add(sample);
	}

	void Document::advancePathStroke(bool finished) {
		TP_PROFILE_SCOPE("brush.path");
		if (!mPathActive || !mActive) return;
		const halnf end = mStrokePath.endParameter(finished);
		const int4 level = mPathStyle.brush.smudges() ? -1 : deferredLevel(mPathStyle.brush.markerBlend());
		if (level <= 0) {
			advanceDensePath(end);
			return;
		}
		usePreviewLevel(level);
		if (mDenseWalk) {
			mRangeStart = { *mDenseWalk, nullptr };
			mPreviewWalk = *mDenseWalk;
			mDenseWalk.reset();
		}
		advancePreviewPath(level, end);
		mPathRangeOpen = true;
		mPathRangeTo = std::max(mPathRangeTo, end);
	}

	void Document::advanceDensePath(halnf end) {
		if (!mDenseWalk) {
			flushDeferredDabs();
			mDenseWalk = mRangeStart.walk ? *mRangeStart.walk : mRangeStart.after->endWalk();
			mRangeStart = {};
		}
		const Brush& brush = mPathStyle.brush;
		mStrokePath.advanceTo(
			*mDenseWalk,
			end,
			[&brush](halnf pressure) { return brush.dabDistance(pressure); },
			[this](const StrokePath::Sample& sample) {
				if (mPathStyle.brush.smudges()) {
					stampBrushDab(sample.position, mPathStyle.brush, sample.pressure);
					return;
				}
				mPathStyle.forEachDab(sample.position, sample.pressure, [this](TracedDab dab) {
					TP_PROFILE_COUNT("brush.dabs", 1);
					const int4 level = deferredLevel(dab.work->marker);
					if (level < 0) {
						if (mPreviewLevel >= 0) settlePaint();
						stageEagerDab(dab);
						return;
					}
					usePreviewLevel(level);
					openTrace().record(dab);
					stagePreviewDab(dab);
				});
			}
		);
		mPreviewWalk = *mDenseWalk;
	}

	void Document::advancePreviewPath(int4 level, halnf end) {
		const halnf scale = (halnf) (1 << level);
		const Brush& brush = mPathStyle.brush;
		mStrokePath.advanceTo(
			mPreviewWalk,
			end,
			[&brush, scale](halnf pressure) { return std::max(brush.dabDistance(pressure), kPreviewPathStep * scale); },
			[this](const StrokePath::Sample& sample) {
				mPathStyle.forEachDab(sample.position, sample.pressure, [this](TracedDab dab) {
					TP_PROFILE_COUNT("brush.preview_dabs", 1);
					paintPreviewDab(dab);
				});
			}
		);
	}

	void Document::resetPathStroke() {
		mPathActive = false;
		mStrokePath = StrokePath();
		mDenseWalk = StrokePath::Walk{};
		mPreviewWalk = {};
		mPathRangeOpen = false;
		mPathRangeTo = 0;
		mRangeStart = {};
	}

	bool Document::previewNeedsDab(const DabWork& work, int4 level) {
		const halnf scale = (halnf) (1 << level);
		const DabShape& shape = work.dab.shape();
		const bool sameShape =
			mLastPreviewDab && std::abs(mLastPreviewDab->dab.shape().size - shape.size) <= kPreviewDabStep * scale &&
			mLastPreviewDab->dab.shape().hardness == shape.hardness &&
			std::abs(mLastPreviewDab->strength - work.strength) < 1.0f / 64.0f && mLastPreviewDab->erase == work.erase;
		if (sameShape) {
			const Vec2F moved = (work.center - mLastPreviewDab->center) / scale;
			if (moved.x * moved.x + moved.y * moved.y < kPreviewDabStep * kPreviewDabStep) return false;
		}
		mLastPreviewDab = std::make_shared<DabWork>(work);
		return true;
	}

	void Document::stagePreviewDab(const TracedDab& dab) {
		if (mPreviewLevel > 0 && !previewNeedsDab(*dab.work, mPreviewLevel)) return;
		paintPreviewDab(dab);
	}

	void Document::paintPreviewDab(const TracedDab& traced) {
		TP_PROFILE_SCOPE("brush.preview_dab");
		const int4 level = mPreviewLevel;
		const int4 span = kTileSize << level;
		const std::shared_ptr<const DabWork>& work = traced.work;
		const int4 minX = traced.minX;
		const int4 minY = traced.minY;
		const int4 maxX = traced.maxX;
		const int4 maxY = traced.maxY;
		LayerLevels& levels = mActive->levels();
		for (int4 ty = floordiv(minY, span); ty <= floordiv(maxY, span); ty++) {
			for (int4 tx = floordiv(minX, span); tx <= floordiv(maxX, span); tx++) {
				const TileCoord coord = { tx, ty };
				if (level == 0) {
					stageBasePreview(*work, coord, minX, minY, maxX, maxY);
					continue;
				}
				const bool fresh = !levels.isPreview(level, coord);
				StrokeMask* mask = nullptr;
				if (traced.masked()) {
					auto found = mPreviewMasks.presents(coord.key());
					if (!found.isValid()) {
						auto created = std::make_shared<StrokeMask>();
						Tile* existing = levels.tileAt(level, coord);
						if (existing) {
							auto base = std::make_shared<Tile>(Tile::Uninitialized{});
							std::memcpy(base->pixels(), existing->pixels(), (size_t) kTileSize * kTileSize * 4);
							created->setBase(base);
						}
						mPreviewMasks.put(coord.key(), created);
						found = mPreviewMasks.presents(coord.key());
					}
					mask = mPreviewMasks.getSlotVal(found).get();
				}
				Tile* tile = levels.previewTile(level, coord);
				if (!tile) continue;
				if (fresh && !mask && !levels.tileAt(level, coord)) continue;
				const int4 originX = coord.x * kTileSize;
				const int4 originY = coord.y * kTileSize;
				const int4 x0 = std::max(0, floordiv(minX, 1 << level) - originX);
				const int4 y0 = std::max(0, floordiv(minY, 1 << level) - originY);
				const int4 x1 = std::min(kTileSize, floordiv(maxX + (1 << level) - 1, 1 << level) + 1 - originX);
				const int4 y1 = std::min(kTileSize, floordiv(maxY + (1 << level) - 1, 1 << level) + 1 - originY);
				if (x1 <= x0 || y1 <= y0) continue;
				StrokePainter::paintAtLevel(*work, level, *tile, coord, x0, y0, x1, y1, mask);
				tile->markDirty({ (halnf) x0, (halnf) y0, (halnf) (x1 - x0), (halnf) (y1 - y0) });
				TP_PROFILE_COUNT("brush.preview_pixels", (x1 - x0) * (y1 - y0));
			}
		}
	}

	void
	Document::stageBasePreview(const DabWork& work, const TileCoord& coord, int4 minX, int4 minY, int4 maxX, int4 maxY) {
		LayerLevels& levels = mActive->levels();
		const bool fresh = !levels.isPreview(0, coord);
		if (fresh && work.keepAlpha && !mActive->tileIfExists(coord)) return;
		const bool masked = !work.erase && !work.marker && !work.keepAlpha;
		Tile* tile = levels.basePreviewTile(*mActive, coord);
		StrokeMask* mask = nullptr;
		if (masked) {
			auto found = mPreviewMasks.presents(coord.key());
			if (!found.isValid()) {
				auto created = std::make_shared<StrokeMask>();
				if (fresh) {
					if (std::shared_ptr<Tile> source = mActive->sharedTile(coord)) created->setBase(source);
				} else {
					auto base = std::make_shared<Tile>(Tile::Uninitialized{});
					std::memcpy(base->pixels(), tile->pixels(), (size_t) kTileSize * kTileSize * 4);
					created->setBase(base);
				}
				mPreviewMasks.put(coord.key(), created);
				found = mPreviewMasks.presents(coord.key());
			}
			mask = mPreviewMasks.getSlotVal(found).get();
		}
		const int4 originX = coord.x * kTileSize;
		const int4 originY = coord.y * kTileSize;
		const int4 x0 = std::max(0, minX - originX);
		const int4 y0 = std::max(0, minY - originY);
		const int4 x1 = std::min(kTileSize, maxX - originX);
		const int4 y1 = std::min(kTileSize, maxY - originY);
		if (x1 <= x0 || y1 <= y0) return;
		StrokePainter::paintAtLevel(work, 0, *tile, coord, x0, y0, x1, y1, mask);
		tile->markDirty({ (halnf) x0, (halnf) y0, (halnf) (x1 - x0), (halnf) (y1 - y0) });
		mActive->noteDisplayChanged(coord);
		TP_PROFILE_COUNT("brush.preview_pixels", (x1 - x0) * (y1 - y0));
	}

	namespace {
		class StrokeTileOperation : public Operation {
		public:
			explicit StrokeTileOperation(std::shared_ptr<StrokeTrace> trace) :
				mTrace(std::move(trace)) {}

			[[nodiscard]] const char* name() const override { return "stroke tile"; }

			struct StrokeCoverage {
				std::mutex mutex;
				std::unique_ptr<StrokeMask> mask;
			};

			void setCancelled(std::shared_ptr<std::atomic<bool>> cancelled) { mCancelled = std::move(cancelled); }

			void setCoverage(const TileCoord& coord, std::shared_ptr<StrokeCoverage> coverage) {
				std::lock_guard lock(mMutex);
				mCoverage.put(coord.key(), std::move(coverage));
			}

			struct Slot {
				std::mutex mutex;
				std::shared_ptr<Tile> tile;
				bool ready = true;
				bool removed = false;
			};

			void setSource(
				const TileCoord& coord,
				std::shared_ptr<Tile> source,
				std::shared_ptr<Slot> previous,
				std::shared_ptr<Slot> output
			) {
				std::lock_guard lock(mMutex);
				mSources.put(coord.key(), std::move(source));
				mPrevious.put(coord.key(), std::move(previous));
				mOutputs.put(coord.key(), std::move(output));
			}

			void run(const TileWork& work, Tile& target) override {
				(void) work;
				(void) target;
			}

			void run(const TileWork& work, const std::shared_ptr<Tile>& result) override {
				Tile& target = *result;
				if (mCancelled && mCancelled->load()) {
					std::shared_ptr<Slot> output;
					{
						std::lock_guard lock(mMutex);
						output = mOutputs.get(work.coord.key());
					}
					if (output) {
						std::lock_guard lock(output->mutex);
						output->ready = true;
					}
					return;
				}
				std::shared_ptr<Tile> source;
				std::shared_ptr<Slot> previous;
				std::shared_ptr<StrokeCoverage> coverage;
				{
					std::lock_guard lock(mMutex);
					source = mSources.get(work.coord.key());
					previous = mPrevious.get(work.coord.key());
					auto found = mCoverage.presents(work.coord.key());
					if (found.isValid()) coverage = mCoverage.getSlotVal(found);
				}
				mTrace->expand();
				const std::vector<ualni>* dabs = mTrace->dabsFor(work.coord);
				if (previous) {
					std::lock_guard lock(previous->mutex);
					source = previous->removed ? nullptr : previous->tile;
				}
				{
					std::lock_guard lock(mMutex);
					mInputs.put(work.coord.key(), source);
				}
				if (source) std::memcpy(target.pixels(), source->pixels(), (size_t) kTileSize * kTileSize * 4);
				else std::memset(target.pixels(), 0, (size_t) kTileSize * kTileSize * 4);
				const int4 originX = work.coord.x * kTileSize;
				const int4 originY = work.coord.y * kTileSize;
				bool erases = false;
				std::unique_ptr<StrokeMask> ownMask;
				std::unique_lock<std::mutex> coverageLock;
				StrokeMask* mask = nullptr;
				if (coverage) {
					coverageLock = std::unique_lock(coverage->mutex);
					mask = coverage->mask.get();
				}
				static const std::vector<ualni> kNone;
				for (const ualni index : dabs ? *dabs : kNone) {
					const TracedDab& dab = mTrace->dab(index);
					const int4 x0 = std::max(0, dab.minX - originX);
					const int4 y0 = std::max(0, dab.minY - originY);
					const int4 x1 = std::min(kTileSize, dab.maxX - originX);
					const int4 y1 = std::min(kTileSize, dab.maxY - originY);
					if (x1 <= x0 || y1 <= y0) continue;
					if (dab.masked()) {
						if (!mask) {
							auto created = std::make_unique<StrokeMask>();
							created->setBase(source);
							mask = created.get();
							if (coverage) coverage->mask = std::move(created);
							else ownMask = std::move(created);
						}
						StrokePainter::paintMasked(*dab.work, *mask, target, work.coord, x0, y0, x1, y1);
					} else {
						StrokePainter::paint(*dab.work, target, work.coord, x0, y0, x1, y1);
					}
					erases = erases || dab.work->erase;
				}
				const bool transparent = erases && target.isTransparent();
				std::shared_ptr<Slot> output;
				{
					std::lock_guard lock(mMutex);
					if (transparent) mTransparent.put(work.coord.key(), true);
					output = mOutputs.get(work.coord.key());
				}
				if (output) {
					std::lock_guard lock(output->mutex);
					output->tile = result;
					output->removed = transparent;
					output->ready = true;
				}
			}

			void finish(Layer& layer, const TileWork& work, const std::shared_ptr<Tile>& result) override {
				if (mCancelled && mCancelled->load()) {
					std::lock_guard lock(mMutex);
					mSources.remove(work.coord.key());
					if (mInputs.presents(work.coord.key()).isValid()) mInputs.remove(work.coord.key());
					mPrevious.remove(work.coord.key());
					mOutputs.remove(work.coord.key());
					if (mCoverage.presents(work.coord.key()).isValid()) mCoverage.remove(work.coord.key());
					return;
				}
				std::shared_ptr<Tile> source;
				std::shared_ptr<Tile> submitted;
				bool transparent = false;
				{
					std::lock_guard lock(mMutex);
					source = mInputs.get(work.coord.key());
					submitted = mSources.get(work.coord.key());
					transparent = mTransparent.presents(work.coord.key()).isValid();
					mSources.remove(work.coord.key());
					mInputs.remove(work.coord.key());
					mPrevious.remove(work.coord.key());
					mOutputs.remove(work.coord.key());
					mCoverage.remove(work.coord.key());
				}
				patchHistory(layer, work.coord, source, submitted, transparent ? nullptr : result);
				if (layer.sharedTile(work.coord) != source) return;
				if (transparent) {
					if (source) layer.removeTile(work.coord);
					return;
				}
				layer.replaceTile(work.coord, result);
				Operation::finish(layer, work, result);
			}

			using Patch = std::function<void(
				Layer&,
				const TileCoord&,
				const std::shared_ptr<Tile>&,
				const std::shared_ptr<Tile>&,
				const std::shared_ptr<Tile>&
			)>;
			Patch patchHistory = [](
														 Layer&,
														 const TileCoord&,
														 const std::shared_ptr<Tile>&,
														 const std::shared_ptr<Tile>&,
														 const std::shared_ptr<Tile>&
													 ) {};

		private:
			std::shared_ptr<StrokeTrace> mTrace;
			Map<ualni, std::shared_ptr<Tile>> mSources;
			Map<ualni, std::shared_ptr<Tile>> mInputs;
			Map<ualni, std::shared_ptr<Slot>> mPrevious;
			Map<ualni, std::shared_ptr<Slot>> mOutputs;
			std::mutex mMutex;
			Map<ualni, bool> mTransparent;
			Map<ualni, std::shared_ptr<StrokeCoverage>> mCoverage;
			std::shared_ptr<std::atomic<bool>> mCancelled;
		};
	}

	void Document::flushDeferredDabs() const {
		auto& self = const_cast<Document&>(*this);
		self.closePathRange();
		if (!mOpenTrace || mOpenTrace->isEmpty()) return;
		TP_PROFILE_SCOPE("brush.flush_deferred");
		std::shared_ptr<StrokeTrace> trace = std::move(self.mOpenTrace);
		Layer* layer = mStrokeLayer;
		if (layer) {
			auto operation = std::make_shared<StrokeTileOperation>(trace);
			if (!self.mStrokeCancelled) self.mStrokeCancelled = std::make_shared<std::atomic<bool>>(false);
			operation->setCancelled(self.mStrokeCancelled);
			auto pendingEntry = std::make_shared<ualni>(0);
			self.mPendingStrokeEntries.push_back(pendingEntry);
			operation->patchHistory = [&self, pendingEntry](
																	Layer& target,
																	const TileCoord& coord,
																	const std::shared_ptr<Tile>& input,
																	const std::shared_ptr<Tile>& submitted,
																	const std::shared_ptr<Tile>& result
																) {
				if (*pendingEntry == 0) return;
				self.mHistory.forEachEntryFrom(*pendingEntry, [&](auto& entry) {
					for (Layer::State& layerState : entry.state.layers) {
						if (layerState.id != target.id()) continue;
						auto found = layerState.tiles.presents(coord.key());
						if (found.isValid()) {
							const std::shared_ptr<Tile>& stored = layerState.tiles.getSlotVal(found).tile;
							if (stored != input && stored != submitted) continue;
							if (result) layerState.tiles.getSlotVal(found).tile = result;
							else layerState.tiles.remove(coord.key());
						} else if (!input && !submitted && result) {
							layerState.tiles.put(coord.key(), { coord, result });
						}
					}
				});
			};
			const bool keepAlpha = trace->keepsAlpha();
			ualni submitted = 0;
			for (const TileCoord& coord : trace->tiles()) {
				std::shared_ptr<Tile> source = layer->sharedTile(coord);
				const ualni chainKey = coord.key() ^ ((ualni) (uintptr_t) layer * 0x9E3779B97F4A7C15ull);
				std::shared_ptr<StrokeTileOperation::Slot> previous;
				auto pending = self.mPendingSlots.presents(chainKey);
				if (pending.isValid()) {
					previous = std::static_pointer_cast<StrokeTileOperation::Slot>(self.mPendingSlots.getSlotVal(pending));
					if (!self.mEngine.hasWork(*layer, coord)) previous.reset();
				}
				if (keepAlpha && !source && !previous) continue;
				auto output = std::make_shared<StrokeTileOperation::Slot>();
				output->ready = false;
				self.mPendingSlots.put(chainKey, output);
				operation->setSource(coord, source, previous, output);
				auto coverage = self.mStrokeCoverage.presents(chainKey);
				if (!coverage.isValid()) {
					self.mStrokeCoverage.put(chainKey, std::make_shared<StrokeTileOperation::StrokeCoverage>());
					coverage = self.mStrokeCoverage.presents(chainKey);
				}
				operation->setCoverage(
					coord,
					std::static_pointer_cast<StrokeTileOperation::StrokeCoverage>(self.mStrokeCoverage.getSlotVal(coverage))
				);
				self.mEngine.submit({ operation, nullptr, layer, { coord, 0 } });
				submitted++;
			}
			TP_PROFILE_COUNT("brush.deferred_tiles", submitted);
		}
	}

	void Document::closePathRange() const {
		if (!mPathRangeOpen) return;
		auto& self = const_cast<Document&>(*this);
		StrokeTrace::PathRange range;
		range.style = mPathStyle;
		range.path = StrokePath(mStrokePath.samples());
		range.fromParameter = mRangeStart.walk ? mRangeStart.walk->parameter : 0.0f;
		range.toParameter = mPathRangeTo;
		range.from = mRangeStart.walk;
		range.previous = mRangeStart.after;
		self.openTrace().setRange(std::move(range));
		self.mRangeStart = { std::nullopt, mOpenTrace };
		self.mPathRangeOpen = false;
	}

	void Document::setPaintWorkers(size_t count) { mPainter.setWorkers(count); }

	void Document::submitPaint() {
		TP_PROFILE_SCOPE("brush.submit");
		mPainter.submit();
		TP_PROFILE_SCOPE("engine.apply");
		if (mEngine.threads() == 0) mEngine.runFor(kEngineSliceMs);
		else mEngine.applyFor(Engine::kFrameApplyMs);
		releaseFinishedPreviews();
	}

	void Document::releaseFinishedPreviews() const {
		if (mPathRangeOpen || (mOpenTrace && !mOpenTrace->isEmpty()) || mEngine.pending() > 0) return;
		mPendingSlots.removeAll();
		for (const auto& layer : mLayers)
			if (layer->levels().previewCount() > 0) layer->levels().endPreview();
	}

	void Document::settlePaint() const {
		flushDeferredDabs();
		mPainter.flush();
		mEngine.drain();
		releaseFinishedPreviews();
	}
}
