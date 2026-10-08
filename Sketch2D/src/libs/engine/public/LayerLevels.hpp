#pragma once

#include "Layer.hpp"
#include "ParallelFor.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace tp::sketch2d {

	class LayerLevels {
	public:
		static constexpr int4 kLevels = 7;

		[[nodiscard]] static int4 levelForZoom(halnf zoom);
		[[nodiscard]] static TileCoord parentOf(const TileCoord& coord) {
			return { floordiv(coord.x, 2), floordiv(coord.y, 2) };
		}

		using PendingCheck = std::function<bool(const TileCoord& coord)>;

		void update(const Layer& layer, int4 upToLevel, ParallelFor* parallel = nullptr);
		void setPendingCheck(PendingCheck isPending) { mIsPending = std::move(isPending); }
		[[nodiscard]] bool isStale(int4 level, const TileCoord& coord) const;
		[[nodiscard]] ualni staleCount() const;
		[[nodiscard]] ualni rebuildCount() const { return mRebuilds; }

		[[nodiscard]] Tile* tileAt(int4 level, const TileCoord& coord) const;
		[[nodiscard]] ualni tileCount(int4 level) const;
		[[nodiscard]] int4 builtLevels() const { return mBuilt; }
		[[nodiscard]] ualni lastDownsampledTiles() const { return mLastDownsampled; }
		[[nodiscard]] ualni lastDownsampledPixels() const { return mLastPixels; }
		[[nodiscard]] const std::vector<TileCoord>& lastChanged(int4 level) const;
		template <typename Fn>
		void takeChanges(int4 level, Fn&& fn) {
			if (level < 1 || level >= kLevels) return;
			for (auto it : mAccumulated[level])
				fn(it->val);
			mAccumulated[level].removeAll();
		}
		[[nodiscard]] const std::vector<std::shared_ptr<Tile>>& lastRemoved() const { return mRemoved; }

		template <typename Fn>
		void forEachTile(int4 level, Fn&& fn) const {
			if (level < 1 || level > mBuilt) return;
			for (auto it : mLevels[(size_t) level - 1])
				fn(it->val.coord, it->val.tile.get());
		}

		static void downsample(const Tile* const quadrants[4], Tile& target, int4 x0, int4 y0, int4 x1, int4 y1);

		Tile* previewTile(int4 level, const TileCoord& coord);
		Tile* basePreviewTile(const Layer& layer, const TileCoord& coord);
		[[nodiscard]] Tile* basePreview(const TileCoord& coord) const;
		template <typename Fn>
		void forEachBasePreview(Fn&& fn) const {
			for (auto it : mBase)
				fn(it->val.coord, it->val.tile.get());
		}
		[[nodiscard]] bool isPreview(int4 level, const TileCoord& coord) const;
		[[nodiscard]] ualni previewCount() const;
		void endPreview();
		void discardPreview();

	private:
		[[nodiscard]] std::vector<TileCoord> holdBackPending(std::vector<TileCoord> changed);
		void markStale(const TileCoord& child);
		[[nodiscard]] bool anyChildPending(const TileCoord& parent) const;

		struct LevelTile {
			TileCoord coord;
			std::shared_ptr<Tile> tile;
		};
		using Level = Map<ualni, LevelTile>;

		template <typename Lookup>
		void updateLevel(
			int4 level,
			const Lookup& children,
			const std::vector<TileCoord>& changedChildren,
			std::vector<TileCoord>& changedParents,
			ParallelFor* parallel
		);

		Level mLevels[kLevels - 1];
		std::vector<TileCoord> mChanged[kLevels];
		int4 mBuilt = 0;
		ualni mLastDownsampled = 0;
		ualni mLastPixels = 0;
		std::vector<std::shared_ptr<Tile>> mRemoved;
		std::vector<TileCoord> mPending;
		Map<ualni, TileCoord> mAccumulated[kLevels];
		Map<ualni, TileCoord> mPreview[kLevels];
		Map<ualni, LevelTile> mOriginals[kLevels];
		Level mBase;
		std::vector<TileCoord> mReleased[kLevels];
		Map<ualni, TileCoord> mStale[kLevels];
		PendingCheck mIsPending;
		ualni mRebuilds = 0;
	};
}
