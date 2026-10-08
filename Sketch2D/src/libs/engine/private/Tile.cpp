#include "Tile.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace tp::sketch2d {

	namespace {
		int4 clampi(int4 v, int4 lo, int4 hi) { return v < lo ? lo : (v > hi ? hi : v); }
	}

	Tile::Tile() {
		const ualni bytes = (ualni) kTileSize * kTileSize * 4;
		mPixels = (uint1*) std::malloc(bytes);
		std::memset(mPixels, 0, bytes);
	}

	Tile::Tile(Uninitialized) { mPixels = (uint1*) std::malloc((ualni) kTileSize * kTileSize * 4); }

	Tile::~Tile() { std::free(mPixels); }

	std::shared_ptr<Tile> Tile::takeForWriting() {
		auto copy = std::make_shared<Tile>();
		std::memcpy(copy->mPixels, mPixels, (ualni) kTileSize * kTileSize * 4);
		copy->mDirty = mDirty;
		copy->mMipChanges = mMipChanges;
		copy->mGpuId = mGpuId;
		mGpuId = 0;
		copy->mCompositeBands = mCompositeBands;
		mDirty = { 0, 0, (halnf) kTileSize, (halnf) kTileSize };
		mMipChanges = mDirty;
		mCompositeBands.include(0, 0, kTileSize, kTileSize);
		return copy;
	}

	void Tile::clear(RGBA color) {
		const uint1 r = (uint1) std::lround(color.r * color.a * 255.0f);
		const uint1 g = (uint1) std::lround(color.g * color.a * 255.0f);
		const uint1 b = (uint1) std::lround(color.b * color.a * 255.0f);
		const uint1 a = (uint1) std::lround(color.a * 255.0f);

		for (int4 i = 0; i < kTileSize * kTileSize; i++) {
			mPixels[i * 4 + 0] = r;
			mPixels[i * 4 + 1] = g;
			mPixels[i * 4 + 2] = b;
			mPixels[i * 4 + 3] = a;
		}
		mDirty = { 0, 0, (halnf) kTileSize, (halnf) kTileSize };
		mMipChanges = mDirty;
		mCompositeBands.include(0, 0, kTileSize, kTileSize);
	}

	bool Tile::isTransparent() const {
		for (ualni index = 3; index < (ualni) kTileSize * kTileSize * 4; index += 4) {
			if (mPixels[index] != 0) return false;
		}
		return true;
	}

	namespace {
		void includeRect(RectF& accumulated, int4 x0, int4 y0, int4 x1, int4 y1) {
			if (accumulated.z <= 0 || accumulated.w <= 0) {
				accumulated = { (halnf) x0, (halnf) y0, (halnf) (x1 - x0), (halnf) (y1 - y0) };
				return;
			}

			const int4 ex0 = (int4) accumulated.pos.x;
			const int4 ey0 = (int4) accumulated.pos.y;
			const int4 ex1 = ex0 + (int4) accumulated.size.x;
			const int4 ey1 = ey0 + (int4) accumulated.size.y;
			const int4 nx0 = x0 < ex0 ? x0 : ex0;
			const int4 ny0 = y0 < ey0 ? y0 : ey0;
			const int4 nx1 = x1 > ex1 ? x1 : ex1;
			const int4 ny1 = y1 > ey1 ? y1 : ey1;
			accumulated = { (halnf) nx0, (halnf) ny0, (halnf) (nx1 - nx0), (halnf) (ny1 - ny0) };
		}
	}

	void Tile::markDirty(const RectF& rectInTile) {
		const int4 x0 = clampi((int4) rectInTile.pos.x, 0, kTileSize);
		const int4 y0 = clampi((int4) rectInTile.pos.y, 0, kTileSize);
		const int4 x1 = clampi((int4) (rectInTile.pos.x + rectInTile.size.x + 0.999f), 0, kTileSize);
		const int4 y1 = clampi((int4) (rectInTile.pos.y + rectInTile.size.y + 0.999f), 0, kTileSize);
		if (x1 <= x0 || y1 <= y0) return;

		includeRect(mDirty, x0, y0, x1, y1);
		includeRect(mMipChanges, x0, y0, x1, y1);
		mCompositeBands.include(x0, y0, x1, y1);
	}

	void DirtySpan::include(int4 left, int4 top, int4 right, int4 bottom) {
		if (right <= left || bottom <= top) return;
		if (isEmpty()) {
			*this = { left, top, right, bottom };
			return;
		}
		x0 = left < x0 ? left : x0;
		y0 = top < y0 ? top : y0;
		x1 = right > x1 ? right : x1;
		y1 = bottom > y1 ? bottom : y1;
	}

	void DirtyBands::include(int4 x0, int4 y0, int4 x1, int4 y1) {
		if (x1 <= x0 || y1 <= y0) return;
		for (int4 band = y0 / kBandRows; band <= (y1 - 1) / kBandRows; band++) {
			const int4 top = band * kBandRows;
			mBands[band].include(x0, y0 > top ? y0 : top, x1, y1 < top + kBandRows ? y1 : top + kBandRows);
		}
	}

	void DirtyBands::include(const DirtyBands& other) {
		for (int4 band = 0; band < kBands; band++) {
			const DirtySpan& span = other.mBands[band];
			mBands[band].include(span.x0, span.y0, span.x1, span.y1);
		}
	}

	void DirtyBands::clear() {
		for (DirtySpan& span : mBands)
			span = {};
	}

	bool DirtyBands::isEmpty() const {
		for (const DirtySpan& span : mBands) {
			if (!span.isEmpty()) return false;
		}
		return true;
	}

	ualni DirtyBands::area() const {
		ualni total = 0;
		for (const DirtySpan& span : mBands) {
			if (!span.isEmpty()) total += (ualni) (span.x1 - span.x0) * (ualni) (span.y1 - span.y0);
		}
		return total;
	}

}
