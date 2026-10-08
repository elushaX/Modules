#include "Selection.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	namespace {
		constexpr int4 kSamples = 4;
	}

	const Selection::MaskTile* Selection::tile(int4 tx, int4 ty) const {
		const auto found = mTiles.find(key(tx, ty));
		return found == mTiles.end() ? nullptr : found->second.get();
	}

	Selection::MaskTile* Selection::writableTile(int4 tx, int4 ty) {
		std::shared_ptr<MaskTile>& slot = mTiles[key(tx, ty)];
		if (!slot) slot = std::make_shared<MaskTile>();
		else if (slot.use_count() > 1) slot = std::make_shared<MaskTile>(*slot);
		return slot.get();
	}

	uint1 Selection::coverage(int4 x, int4 y) const {
		if (!mActive) return 255;
		const MaskTile* mask = tile(floordiv(x, kTileSize), floordiv(y, kTileSize));
		const uint1 value =
			mask
				? mask
						->coverage[(x - floordiv(x, kTileSize) * kTileSize) + (y - floordiv(y, kTileSize) * kTileSize) * kTileSize]
				: 0;
		return mInverted ? (uint1) (255 - value) : value;
	}

	halnf Selection::coverageAt(Vec2F world) const {
		return coverage((int4) std::floor(world.x), (int4) std::floor(world.y)) / 255.0f;
	}

	void Selection::clear() {
		mTiles.clear();
		mActive = false;
		mInverted = false;
	}

	void Selection::combine(int4 x, int4 y, uint1 value, SelectionMode mode) {
		if (mode != SelectionMode::Replace && value == 0) return;
		const int4 tx = floordiv(x, kTileSize);
		const int4 ty = floordiv(y, kTileSize);
		if (mode == SelectionMode::Subtract && !tile(tx, ty)) return;
		MaskTile* mask = writableTile(tx, ty);
		uint1& cell = mask->coverage[(x - tx * kTileSize) + (y - ty * kTileSize) * kTileSize];
		switch (mode) {
			case SelectionMode::Replace: cell = value; break;
			case SelectionMode::Add: cell = std::max(cell, value); break;
			case SelectionMode::Subtract: cell = (uint1) std::max(0, (int4) cell - (int4) value); break;
		}
	}

	void Selection::addMask(int4 left, int4 top, int4 width, const std::vector<uint1>& mask, SelectionMode mode) {
		if (width <= 0) return;
		if (mode == SelectionMode::Replace) {
			mTiles.clear();
			mInverted = false;
			mode = SelectionMode::Add;
		} else if (!mActive) {
			if (mode == SelectionMode::Subtract) return;
			mTiles.clear();
			mInverted = false;
		}
		const SelectionMode stored =
			mInverted ? (mode == SelectionMode::Add ? SelectionMode::Subtract : SelectionMode::Add) : mode;
		for (size_t index = 0; index < mask.size(); index++)
			combine(left + (int4) (index % (size_t) width), top + (int4) (index / (size_t) width), mask[index], stored);
		mActive = true;
		compact();
	}

	void Selection::compact() {
		for (auto it = mTiles.begin(); it != mTiles.end();) {
			const uint1* cells = it->second->coverage;
			if (std::all_of(cells, cells + kTileSize * kTileSize, [](uint1 value) { return value == 0; }))
				it = mTiles.erase(it);
			else ++it;
		}
		if (mTiles.empty() && !mInverted) mActive = false;
	}

	void Selection::selectAll(int4 left, int4 top, int4 right, int4 bottom) {
		clear();
		addRectangle(left, top, right, bottom, SelectionMode::Replace);
	}

	void Selection::invert() {
		if (!mActive) return;
		mInverted = !mInverted;
		if (mTiles.empty() && !mInverted) mActive = false;
	}

	void Selection::addRectangle(int4 left, int4 top, int4 right, int4 bottom, SelectionMode mode) {
		if (right <= left || bottom <= top) {
			if (mode == SelectionMode::Replace) clear();
			return;
		}
		addMask(left, top, right - left, std::vector<uint1>((size_t) (right - left) * (bottom - top), 255), mode);
	}

	void Selection::addEllipse(Vec2F center, Vec2F radius, SelectionMode mode) {
		if (radius.x <= 0 || radius.y <= 0) {
			if (mode == SelectionMode::Replace) clear();
			return;
		}
		const int4 left = (int4) std::floor(center.x - radius.x);
		const int4 top = (int4) std::floor(center.y - radius.y);
		const int4 right = (int4) std::ceil(center.x + radius.x);
		const int4 bottom = (int4) std::ceil(center.y + radius.y);
		const int4 width = right - left;
		std::vector<uint1> mask((size_t) width * (bottom - top), 0);
		for (int4 y = top; y < bottom; y++)
			for (int4 x = left; x < right; x++) {
				int4 inside = 0;
				for (int4 sy = 0; sy < kSamples; sy++)
					for (int4 sx = 0; sx < kSamples; sx++) {
						const halnf dx = ((halnf) x + (sx + 0.5f) / kSamples - center.x) / radius.x;
						const halnf dy = ((halnf) y + (sy + 0.5f) / kSamples - center.y) / radius.y;
						if (dx * dx + dy * dy <= 1) inside++;
					}
				mask[(size_t) (y - top) * width + (x - left)] = (uint1) (inside * 255 / (kSamples * kSamples));
			}
		addMask(left, top, width, mask, mode);
	}

	void Selection::addPolygon(const std::vector<Vec2F>& points, SelectionMode mode) {
		if (points.size() < 3) {
			if (mode == SelectionMode::Replace) clear();
			return;
		}
		halnf minX = points[0].x, minY = points[0].y, maxX = minX, maxY = minY;
		for (const Vec2F& point : points) {
			minX = std::min(minX, point.x);
			minY = std::min(minY, point.y);
			maxX = std::max(maxX, point.x);
			maxY = std::max(maxY, point.y);
		}
		const int4 left = (int4) std::floor(minX);
		const int4 top = (int4) std::floor(minY);
		const int4 right = (int4) std::ceil(maxX);
		const int4 bottom = (int4) std::ceil(maxY);
		const int4 width = std::max(1, right - left);
		const int4 height = std::max(1, bottom - top);
		std::vector<int4> hits((size_t) width * height, 0);
		std::vector<halnf> crossings;
		for (int4 row = 0; row < height * kSamples; row++) {
			const halnf y = (halnf) top + (row + 0.5f) / kSamples;
			crossings.clear();
			for (size_t index = 0; index < points.size(); index++) {
				const Vec2F a = points[index];
				const Vec2F b = points[(index + 1) % points.size()];
				if ((a.y <= y) == (b.y <= y)) continue;
				crossings.push_back(a.x + (y - a.y) / (b.y - a.y) * (b.x - a.x));
			}
			std::sort(crossings.begin(), crossings.end());
			for (size_t pair = 0; pair + 1 < crossings.size(); pair += 2) {
				for (int4 column = 0; column < width * kSamples; column++) {
					const halnf x = (halnf) left + (column + 0.5f) / kSamples;
					if (x >= crossings[pair] && x < crossings[pair + 1])
						hits[(size_t) (row / kSamples) * width + column / kSamples]++;
				}
			}
		}
		std::vector<uint1> mask(hits.size());
		for (size_t index = 0; index < hits.size(); index++)
			mask[index] = (uint1) std::min(255, hits[index] * 255 / (kSamples * kSamples));
		addMask(left, top, width, mask, mode);
	}

	void Selection::spread(int4 pixels) {
		if (!mActive || pixels == 0) return;
		const bool grow = (pixels > 0) != mInverted;
		const int4 steps = std::abs(pixels);
		int4 left = 0, top = 0, right = 0, bottom = 0;
		const bool inverted = mInverted;
		mInverted = false;
		const bool any = bounds(left, top, right, bottom);
		mInverted = inverted;
		if (!any) return;
		left -= steps;
		top -= steps;
		right += steps;
		bottom += steps;
		const int4 width = right - left;
		const int4 height = bottom - top;
		std::vector<uint1> current((size_t) width * height, 0);
		for (int4 y = top; y < bottom; y++)
			for (int4 x = left; x < right; x++) {
				const MaskTile* mask = tile(floordiv(x, kTileSize), floordiv(y, kTileSize));
				current[(size_t) (y - top) * width + (x - left)] =
					mask ? mask->coverage
									 [(x - floordiv(x, kTileSize) * kTileSize) + (y - floordiv(y, kTileSize) * kTileSize) * kTileSize]
							 : 0;
			}
		for (int4 step = 0; step < steps; step++) {
			std::vector<uint1> next = current;
			for (int4 y = 0; y < height; y++)
				for (int4 x = 0; x < width; x++) {
					uint1 value = current[(size_t) y * width + x];
					for (const auto [dx, dy] : { std::pair{ -1, 0 }, std::pair{ 1, 0 }, std::pair{ 0, -1 }, std::pair{ 0, 1 } }) {
						const int4 nx = x + dx;
						const int4 ny = y + dy;
						const uint1 neighbour =
							(nx < 0 || ny < 0 || nx >= width || ny >= height) ? 0 : current[(size_t) ny * width + nx];
						value = grow ? std::max(value, neighbour) : std::min(value, neighbour);
					}
					next[(size_t) y * width + x] = value;
				}
			current = std::move(next);
		}
		mTiles.clear();
		for (size_t index = 0; index < current.size(); index++)
			if (current[index])
				combine(
					left + (int4) (index % (size_t) width),
					top + (int4) (index / (size_t) width),
					current[index],
					SelectionMode::Add
				);
		compact();
		if (!mInverted && mTiles.empty()) mActive = false;
	}

	void Selection::blur(int4 radius) {
		if (!mActive || radius <= 0) return;
		int4 left = 0, top = 0, right = 0, bottom = 0;
		const bool inverted = mInverted;
		mInverted = false;
		const bool any = bounds(left, top, right, bottom);
		mInverted = inverted;
		if (!any) return;
		left -= radius;
		top -= radius;
		right += radius;
		bottom += radius;
		const int4 width = right - left;
		const int4 height = bottom - top;
		std::vector<halnf> values((size_t) width * height, 0);
		for (int4 y = top; y < bottom; y++)
			for (int4 x = left; x < right; x++) {
				const MaskTile* mask = tile(floordiv(x, kTileSize), floordiv(y, kTileSize));
				values[(size_t) (y - top) * width + (x - left)] =
					mask ? mask->coverage
									 [(x - floordiv(x, kTileSize) * kTileSize) + (y - floordiv(y, kTileSize) * kTileSize) * kTileSize]
							 : 0;
			}
		const halnf outside = 0;
		std::vector<halnf> pass(values.size());
		const halnf window = (halnf) (2 * radius + 1);
		for (int4 y = 0; y < height; y++)
			for (int4 x = 0; x < width; x++) {
				halnf sum = 0;
				for (int4 k = -radius; k <= radius; k++) {
					const int4 nx = x + k;
					sum += (nx < 0 || nx >= width) ? outside : values[(size_t) y * width + nx];
				}
				pass[(size_t) y * width + x] = sum / window;
			}
		for (int4 y = 0; y < height; y++)
			for (int4 x = 0; x < width; x++) {
				halnf sum = 0;
				for (int4 k = -radius; k <= radius; k++) {
					const int4 ny = y + k;
					sum += (ny < 0 || ny >= height) ? outside : pass[(size_t) ny * width + x];
				}
				values[(size_t) y * width + x] = sum / window;
			}
		mTiles.clear();
		for (size_t index = 0; index < values.size(); index++) {
			const uint1 value = (uint1) std::clamp((int4) std::lround(values[index]), 0, 255);
			if (value)
				combine(
					left + (int4) (index % (size_t) width), top + (int4) (index / (size_t) width), value, SelectionMode::Add
				);
		}
		compact();
	}

	bool Selection::bounds(int4& left, int4& top, int4& right, int4& bottom) const {
		if (!mActive || mInverted) return false;
		bool any = false;
		for (const auto& [packed, mask] : mTiles) {
			const int4 tx = (int4) (uint4) (packed >> 32);
			const int4 ty = (int4) (uint4) (packed & 0xffffffffu);
			for (int4 y = 0; y < kTileSize; y++)
				for (int4 x = 0; x < kTileSize; x++) {
					if (mask->coverage[x + y * kTileSize] == 0) continue;
					const int4 px = tx * kTileSize + x;
					const int4 py = ty * kTileSize + y;
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
		return any;
	}

	std::vector<std::pair<Vec2F, Vec2F>> Selection::outline() const {
		std::vector<std::pair<Vec2F, Vec2F>> edges;
		if (!mActive) return edges;
		const auto inside = [this](int4 x, int4 y) {
			const MaskTile* mask = tile(floordiv(x, kTileSize), floordiv(y, kTileSize));
			if (!mask) return false;
			return mask->coverage
							 [(x - floordiv(x, kTileSize) * kTileSize) + (y - floordiv(y, kTileSize) * kTileSize) * kTileSize] >= 128;
		};
		for (const auto& [packed, mask] : mTiles) {
			const int4 tx = (int4) (uint4) (packed >> 32);
			const int4 ty = (int4) (uint4) (packed & 0xffffffffu);
			for (int4 y = 0; y < kTileSize; y++)
				for (int4 x = 0; x < kTileSize; x++) {
					if (mask->coverage[x + y * kTileSize] < 128) continue;
					const int4 px = tx * kTileSize + x;
					const int4 py = ty * kTileSize + y;
					const halnf fx = (halnf) px;
					const halnf fy = (halnf) py;
					if (!inside(px, py - 1)) edges.push_back({ { fx, fy }, { fx + 1, fy } });
					if (!inside(px, py + 1)) edges.push_back({ { fx, fy + 1 }, { fx + 1, fy + 1 } });
					if (!inside(px - 1, py)) edges.push_back({ { fx, fy }, { fx, fy + 1 } });
					if (!inside(px + 1, py)) edges.push_back({ { fx + 1, fy }, { fx + 1, fy + 1 } });
				}
		}
		return edges;
	}

	bool Selection::operator==(const Selection& other) const {
		if (mActive != other.mActive || mInverted != other.mInverted || mTiles.size() != other.mTiles.size()) return false;
		for (const auto& [packed, mask] : mTiles) {
			const auto found = other.mTiles.find(packed);
			if (found == other.mTiles.end()) return false;
			if (
				found->second != mask &&
				!std::equal(mask->coverage, mask->coverage + kTileSize * kTileSize, found->second->coverage)
			)
				return false;
		}
		return true;
	}
}
