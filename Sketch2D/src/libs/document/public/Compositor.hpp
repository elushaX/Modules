#pragma once

#include "Document.hpp"
#include "LayerLevels.hpp"
#include "ParallelFor.hpp"

#include <memory>
#include <vector>

namespace tp::sketch2d {

	struct BlendPixel {
		halnf r = 0;
		halnf g = 0;
		halnf b = 0;
		halnf a = 0;
	};

	[[nodiscard]] BlendPixel blendPixel(BlendMode mode, const BlendPixel& below, const BlendPixel& above, halnf opacity);

	class Compositor {
	public:
		static void flatten(const std::vector<const Layer*>& layers, const TileCoord& coord, Tile& target);

		struct StackEntry {
			const Layer* layer;
			Tile* tile;
		};

		static void compositeRows(const std::vector<StackEntry>& stack, uint1* out, int4 x0, int4 x1, int4 y);
		[[nodiscard]] static ualni signatureOf(const std::vector<StackEntry>& stack);
		[[nodiscard]] static bool isVisibleIn(const Document& document, const Layer& layer, bool& baseVisible);

		struct PixelRect {
			int4 x0 = 0;
			int4 y0 = 0;
			int4 x1 = 0;
			int4 y1 = 0;

			[[nodiscard]] bool isEmpty() const { return x1 <= x0 || y1 <= y0; }
			void include(const PixelRect& other);
			static PixelRect whole();
		};

		struct CompositeTile {
			TileCoord coord;
			std::shared_ptr<Tile> tile;
			ualni signature = 0;
			ualni frame = 0;
		};

		void update(const Document& document);
		void update(const Document& document, int4 level);
		[[nodiscard]] int4 level() const { return mLevel; }
		[[nodiscard]] LayerLevels* levelsOf(const Layer& layer) const;
		void setParallel(ParallelFor* parallel) { mParallel = parallel; }

		[[nodiscard]] Tile* tileAt(const TileCoord& coord) const { return tileAt(mLevel, coord); }
		[[nodiscard]] Tile* tileAt(int4 level, const TileCoord& coord) const;
		[[nodiscard]] ualni tileCount() const { return tileCount(mLevel); }
		[[nodiscard]] ualni tileCount(int4 level) const { return mLevels[level].tiles.size(); }
		[[nodiscard]] ualni lastCompositedTiles() const { return mLastComposited; }
		[[nodiscard]] ualni lastCompositedPixels() const { return mLastPixels; }

		template <typename Fn>
		void forEachTile(Fn&& fn) const {
			forEachTile(mLevel, fn);
		}
		template <typename Fn>
		void forEachTile(int4 level, Fn&& fn) const {
			for (auto it : mLevels[level].tiles)
				fn(it->val.coord, it->val.tile.get());
		}

		[[nodiscard]] const std::vector<TileCoord>& lastChanged() const { return mChanged; }
		[[nodiscard]] const std::vector<std::shared_ptr<Tile>>& lastRemoved() const { return mRemoved; }

	private:
		[[nodiscard]] static std::vector<StackEntry> stackAt(const Document& document, const TileCoord& coord);
		[[nodiscard]] static DirtyBands pendingChanges(const std::vector<StackEntry>& stack);
		static void composite(const std::vector<StackEntry>& stack, Tile& target, const PixelRect& rect);
		static void composite(const std::vector<StackEntry>& stack, Tile& target, const DirtyBands& bands);
		[[nodiscard]] static bool isPlainCopy(const std::vector<StackEntry>& stack);
		[[nodiscard]] static bool isNormalStack(const std::vector<StackEntry>& stack);

		void syncLevels(const Document& document, int4 level);
		void updateBase(const Document& document, bool levelChanged);
		void updateLevel(const Document& document, int4 level, bool levelChanged);

		struct LevelCache {
			Map<ualni, CompositeTile> tiles;
			ualni layout = 0;
			bool hasLayout = false;
		};
		LevelCache mLevels[LayerLevels::kLevels];
		int4 mLevel = 0;
		ualni mFrame = 0;
		ualni mLastComposited = 0;
		ualni mLastPixels = 0;
		ParallelFor* mParallel = nullptr;
		std::vector<TileCoord> mChanged;
		std::vector<std::shared_ptr<Tile>> mRemoved;
	};
}
