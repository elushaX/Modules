#include "StrokeTrace.hpp"

#include "DabMask.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	std::optional<TracedDab>
	traceDab(const Brush& brush, const DabShape& requested, const DabTarget& target, Vec2F center) {
		const DabShape dab = DabQuantizer::shape(requested);
		const halnf radius = dab.radius();
		if (radius <= 0.0f) return std::nullopt;
		if (brush.erases() && target.keepAlpha) return std::nullopt;

		auto work = std::make_shared<DabWork>();
		work->dab = PreparedDab(dab);
		if (work->dab.isEmpty()) return std::nullopt;

		const bool masked = !brush.erases() && !brush.markerBlend() && !target.keepAlpha;
		work->center = center;
		work->strength = masked ? dab.flow : dab.flow * dab.opacity;
		work->opacity = masked ? dab.opacity : 1.0f;
		work->source[0] = brush.color.r * 255.0f;
		work->source[1] = brush.color.g * 255.0f;
		work->source[2] = brush.color.b * 255.0f;
		work->erase = brush.erases();
		work->marker = brush.markerBlend();
		work->keepAlpha = target.keepAlpha;
		work->selection = target.selection;

		TracedDab traced;
		traced.minX = (int4) std::floor(center.x - radius);
		traced.minY = (int4) std::floor(center.y - radius);
		traced.maxX = (int4) std::ceil(center.x + radius);
		traced.maxY = (int4) std::ceil(center.y + radius);
		traced.work = std::move(work);
		return traced;
	}

	void StrokeStyle::forEachDab(Vec2F center, halnf pressure, const std::function<void(TracedDab dab)>& emit) const {
		const DabShape shape = brush.dabAt(pressure);
		if (std::optional<TracedDab> dab = traceDab(brush, shape, target, center)) emit(std::move(*dab));
		if (!mirror) return;
		if (std::optional<TracedDab> dab = traceDab(brush, shape, target, mirror(center))) emit(std::move(*dab));
	}

	namespace {
		struct TileBox {
			int4 left = 1;
			int4 top = 1;
			int4 right = 0;
			int4 bottom = 0;

			static TileBox around(Vec2F from, Vec2F to, halnf reach) {
				return { floordiv((int4) std::floor(std::min(from.x, to.x) - reach), kTileSize),
								 floordiv((int4) std::floor(std::min(from.y, to.y) - reach), kTileSize),
								 floordiv((int4) std::ceil(std::max(from.x, to.x) + reach), kTileSize),
								 floordiv((int4) std::ceil(std::max(from.y, to.y) + reach), kTileSize) };
			}

			[[nodiscard]] bool contains(int4 x, int4 y) const { return x >= left && x <= right && y >= top && y <= bottom; }
		};

		void coverTiles(const TileBox& box, TileBox& last, Map<ualni, TileCoord>& tiles) {
			for (int4 ty = box.top; ty <= box.bottom; ty++)
				for (int4 tx = box.left; tx <= box.right; tx++)
					if (!last.contains(tx, ty)) tiles.put(TileCoord{ tx, ty }.key(), { tx, ty });
			last = box;
		}
	}

	void StrokeTrace::record(TracedDab dab) {
		mKeepsAlpha = mKeepsAlpha || dab.work->keepAlpha;
		for (int4 ty = floordiv(dab.minY, kTileSize); ty <= floordiv(dab.maxY - 1, kTileSize); ty++)
			for (int4 tx = floordiv(dab.minX, kTileSize); tx <= floordiv(dab.maxX - 1, kTileSize); tx++)
				mRecordedTiles.put(TileCoord{ tx, ty }.key(), { tx, ty });
		mRecorded++;
		index(std::move(dab));
	}

	void StrokeTrace::setRange(PathRange range) {
		mKeepsAlpha = mKeepsAlpha || range.style.target.keepAlpha;
		mRange = std::move(range);
	}

	void StrokeTrace::index(TracedDab dab) {
		const ualni position = mDabs.size();
		for (int4 ty = floordiv(dab.minY, kTileSize); ty <= floordiv(dab.maxY - 1, kTileSize); ty++)
			for (int4 tx = floordiv(dab.minX, kTileSize); tx <= floordiv(dab.maxX - 1, kTileSize); tx++) {
				const ualni key = TileCoord{ tx, ty }.key();
				auto found = mTileDabs.presents(key);
				if (!found.isValid()) {
					mTileDabs.put(key, {});
					found = mTileDabs.presents(key);
				}
				mTileDabs.getSlotVal(found).push_back(position);
			}
		mDabs.push_back(std::move(dab));
	}

	StrokePath::Walk StrokeTrace::endWalk() {
		expand();
		return mEndWalk;
	}

	void StrokeTrace::expand() {
		std::call_once(mExpanded, [this]() {
			if (!mRange) return;
			PathRange& range = *mRange;
			StrokePath::Walk walk = range.from ? *range.from : range.previous->endWalk();
			range.previous.reset();
			const Brush& brush = range.style.brush;
			range.path.advanceTo(
				walk,
				range.toParameter,
				[&brush](halnf pressure) { return brush.dabDistance(pressure); },
				[this, &range](const StrokePath::Sample& sample) {
					range.style.forEachDab(sample.position, sample.pressure, [this](TracedDab dab) { index(std::move(dab)); });
				}
			);
			mEndWalk = walk;
		});
	}

	const std::vector<ualni>* StrokeTrace::dabsFor(const TileCoord& coord) const {
		auto found = mTileDabs.presents(coord.key());
		return found.isValid() ? &mTileDabs.getSlotVal(found) : nullptr;
	}

	std::vector<TileCoord> StrokeTrace::tiles() const {
		Map<ualni, TileCoord> tiles = mRecordedTiles;
		collectRangeTiles(tiles);
		std::vector<TileCoord> result;
		result.reserve(tiles.size());
		for (auto it : tiles)
			result.push_back(it->val);
		return result;
	}

	void StrokeTrace::collectRangeTiles(Map<ualni, TileCoord>& tiles) const {
		if (!mRange) return;
		const PathRange& range = *mRange;
		const halnf reach = range.style.brush.dabAt(1.0f).radius() + 2.0f;
		TileBox last;
		TileBox mirrored;
		range.path.forEachPoint(range.fromParameter, range.toParameter, [&](Vec2F from, Vec2F to) {
			coverTiles(TileBox::around(from, to, reach), last, tiles);
			if (range.style.mirror)
				coverTiles(TileBox::around(range.style.mirror(from), range.style.mirror(to), reach), mirrored, tiles);
		});
	}
}
