#pragma once

#include "Selection.hpp"
#include "Brush.hpp"
#include "DabMask.hpp"
#include "StrokeMask.hpp"
#include "Tile.hpp"
#include "WorkerLanes.hpp"

#include <atomic>
#include <thread>
#include <memory>
#include <vector>

namespace tp::sketch2d {

	class Layer;

	struct DabWork {
		PreparedDab dab;
		Vec2F center = { 0, 0 };
		halnf strength = 0;
		halnf opacity = 1;
		halnf source[3] = { 0, 0, 0 };
		bool erase = false;
		bool marker = false;
		bool keepAlpha = false;
		std::shared_ptr<const Selection> selection;
	};

	struct DabJob {
		std::shared_ptr<const DabWork> work;
		Tile* tile = nullptr;
		StrokeMask* mask = nullptr;
		Layer* layer = nullptr;
		TileCoord coord;
		int4 x0 = 0;
		int4 y0 = 0;
		int4 x1 = 0;
		int4 y1 = 0;
	};

	class StrokePainter {
	public:
		static constexpr int4 kBandRows = DirtyBands::kBandRows;

		StrokePainter() = default;
		~StrokePainter();

		StrokePainter(const StrokePainter&) = delete;
		StrokePainter& operator=(const StrokePainter&) = delete;

		void setWorkers(size_t count);
		[[nodiscard]] size_t workers() const { return mLanes ? mLanes->laneCount() : 0; }

		void stage(
			const std::shared_ptr<const DabWork>& work,
			Tile& tile,
			Layer& layer,
			TileCoord coord,
			int4 x0,
			int4 y0,
			int4 x1,
			int4 y1,
			StrokeMask* mask = nullptr
		);
		void submit();
		void flush();
		[[nodiscard]] size_t backlog() const { return mStagedCount + (mLanes ? mLanes->pending() : 0); }

		template <typename Fn>
		void whilePaused(Fn&& fn) {
			if (!mLanes || isPausedHere()) {
				fn();
				return;
			}
			auto pause = mLanes->pause();
			mPausedBy.store(std::this_thread::get_id());
			applyNotes();
			fn();
			mPausedBy.store(std::thread::id());
		}

		[[nodiscard]] bool isPausedHere() const { return mPausedBy.load() == std::this_thread::get_id(); }

		[[nodiscard]] double takeWorkerMs();

		static void paint(const DabWork& work, Tile& tile, TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1);
		static void
		paintMasked(const DabWork& work, StrokeMask& mask, Tile& tile, TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1);
		static void paintAtLevel(
			const DabWork& work,
			int4 level,
			Tile& tile,
			TileCoord coord,
			int4 x0,
			int4 y0,
			int4 x1,
			int4 y1,
			StrokeMask* mask = nullptr
		);

	private:
		struct DirtyNote {
			Tile* tile;
			Layer* layer;
			TileCoord coord;
			int4 x0;
			int4 y0;
			int4 x1;
			int4 y1;
		};

		void run(size_t lane, DabJob& job);
		void applyNotes();
		[[nodiscard]] size_t laneOf(TileCoord coord, int4 bandY) const;

		std::unique_ptr<WorkerLanes<DabJob>> mLanes;
		std::vector<std::vector<DabJob>> mStaged;
		std::vector<std::vector<DirtyNote>> mNotes;
		size_t mStagedCount = 0;
		std::atomic<int64_t> mWorkerNs{ 0 };
		std::atomic<std::thread::id> mPausedBy{};
	};
}
