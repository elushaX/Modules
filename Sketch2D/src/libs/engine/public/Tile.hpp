#pragma once

#include "Environment.hpp"
#include "Vec.hpp"
#include "Rect.hpp"
#include "Color.hpp"

#include <memory>

namespace tp::sketch2d {

#ifndef SKETCH2D_TILE_SIZE
#define SKETCH2D_TILE_SIZE 128
#endif

	constexpr int4 kTileSize = SKETCH2D_TILE_SIZE;
	static_assert(
		kTileSize >= 64 && (kTileSize & (kTileSize - 1)) == 0, "tile size must be a power of two of at least 64"
	);

	[[nodiscard]] constexpr int4 floordiv(int4 value, int4 divisor) {
		const int4 quotient = value / divisor;
		return (value % divisor != 0 && (value < 0) != (divisor < 0)) ? quotient - 1 : quotient;
	}

	struct TileCoord {
		int4 x = 0;
		int4 y = 0;

		bool operator==(const TileCoord& o) const { return x == o.x && y == o.y; }
		bool operator!=(const TileCoord& o) const { return !(*this == o); }

		[[nodiscard]] ualni key() const {
			const uint4 ux = (uint4) (x + (int4) 0x40000000);
			const uint4 uy = (uint4) (y + (int4) 0x40000000);
			return ((ualni) ux << 32) | (ualni) uy;
		}
	};

	struct DirtySpan {
		int4 x0 = 0;
		int4 y0 = 0;
		int4 x1 = 0;
		int4 y1 = 0;

		[[nodiscard]] bool isEmpty() const { return x1 <= x0 || y1 <= y0; }
		void include(int4 left, int4 top, int4 right, int4 bottom);
	};

	class DirtyBands {
	public:
		static constexpr int4 kBandRows = 32;
		static constexpr int4 kBands = kTileSize / kBandRows;

		void include(int4 x0, int4 y0, int4 x1, int4 y1);
		void include(const DirtyBands& other);
		void clear();
		[[nodiscard]] bool isEmpty() const;
		[[nodiscard]] const DirtySpan& band(int4 index) const { return mBands[(size_t) index]; }
		[[nodiscard]] ualni area() const;

	private:
		DirtySpan mBands[kBands];
	};

	class Tile {
	public:
		Tile();
		struct Uninitialized {};
		explicit Tile(Uninitialized);
		~Tile();

		Tile(const Tile&) = delete;
		Tile& operator=(const Tile&) = delete;

		[[nodiscard]] std::shared_ptr<Tile> takeForWriting();

		[[nodiscard]] uint1* pixels() { return mPixels; }
		[[nodiscard]] const uint1* pixels() const { return mPixels; }

		void clear(RGBA color);
		[[nodiscard]] bool isTransparent() const;

		void markDirty(const RectF& rectInTile);

		[[nodiscard]] bool hasDirty() const { return mDirty.z > 0 && mDirty.w > 0; }
		[[nodiscard]] const RectF& dirtyRect() const { return mDirty; }
		[[nodiscard]] const DirtyBands& compositeBands() const { return mCompositeBands; }
		void clearDirty() { mDirty = { 0, 0, 0, 0 }; }
		void clearCompositeBands() { mCompositeBands.clear(); }

		[[nodiscard]] bool hasMipChanges() const { return mMipChanges.z > 0 && mMipChanges.w > 0; }
		[[nodiscard]] const RectF& mipChanges() const { return mMipChanges; }
		void clearMipChanges() { mMipChanges = { 0, 0, 0, 0 }; }

		[[nodiscard]] ualni gpuId() const { return mGpuId; }
		void setGpuId(ualni id) { mGpuId = id; }

	private:
		uint1* mPixels = nullptr;
		RectF mDirty = { 0, 0, 0, 0 };
		DirtyBands mCompositeBands;
		RectF mMipChanges = { 0, 0, 0, 0 };
		ualni mGpuId = 0;
	};
}
