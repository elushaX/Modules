#pragma once

#include "Brush.hpp"
#include "StrokePainter.hpp"
#include "StrokePath.hpp"
#include "Tile.hpp"

#include "Map.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace tp::sketch2d {

	using StrokeMirror = std::function<Vec2F(Vec2F point)>;

	struct TracedDab {
		std::shared_ptr<const DabWork> work;
		int4 minX = 0;
		int4 minY = 0;
		int4 maxX = 0;
		int4 maxY = 0;

		[[nodiscard]] bool masked() const { return !work->erase && !work->marker && !work->keepAlpha; }
	};

	struct DabTarget {
		bool keepAlpha = false;
		std::shared_ptr<const Selection> selection;
	};

	[[nodiscard]] std::optional<TracedDab>
	traceDab(const Brush& brush, const DabShape& requested, const DabTarget& target, Vec2F center);

	struct StrokeStyle {
		Brush brush;
		DabTarget target;
		StrokeMirror mirror;

		void forEachDab(Vec2F center, halnf pressure, const std::function<void(TracedDab dab)>& emit) const;
	};

	class StrokeTrace {
	public:
		struct PathRange {
			StrokeStyle style;
			StrokePath path;
			halnf fromParameter = 0;
			halnf toParameter = 0;
			std::optional<StrokePath::Walk> from;
			std::shared_ptr<StrokeTrace> previous;
		};

		StrokeTrace() = default;
		StrokeTrace(const StrokeTrace&) = delete;
		StrokeTrace& operator=(const StrokeTrace&) = delete;

		void record(TracedDab dab);
		void setRange(PathRange range);
		void expand();

		[[nodiscard]] StrokePath::Walk endWalk();
		[[nodiscard]] const std::vector<ualni>* dabsFor(const TileCoord& coord) const;
		[[nodiscard]] const TracedDab& dab(ualni index) const { return mDabs[index]; }
		[[nodiscard]] std::vector<TileCoord> tiles() const;
		[[nodiscard]] bool keepsAlpha() const { return mKeepsAlpha; }
		[[nodiscard]] bool isEmpty() const { return mDabs.empty() && !mRange; }
		[[nodiscard]] size_t recordedDabs() const { return mRecorded; }

	private:
		void index(TracedDab dab);
		void collectRangeTiles(Map<ualni, TileCoord>& tiles) const;

		std::optional<PathRange> mRange;
		std::once_flag mExpanded;
		StrokePath::Walk mEndWalk;
		std::vector<TracedDab> mDabs;
		Map<ualni, std::vector<ualni>> mTileDabs;
		Map<ualni, TileCoord> mRecordedTiles;
		size_t mRecorded = 0;
		bool mKeepsAlpha = false;
	};
}
