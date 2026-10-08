#include "LayerLevels.hpp"

#include <cstring>

#include <algorithm>
#include <cmath>

namespace tp::sketch2d {

	namespace {
		constexpr int4 kHalfTile = kTileSize / 2;
		const std::vector<TileCoord> kNoChanges;
	}

	int4 LayerLevels::levelForZoom(halnf zoom) {
		if (zoom > 0.5f) return 0;
		const int4 level = (int4) std::floor(std::log2(1.0f / zoom) + 1.0e-4f);
		return std::clamp(level, 0, kLevels - 1);
	}

	void LayerLevels::downsample(const Tile* const quadrants[4], Tile& target, int4 x0, int4 y0, int4 x1, int4 y1) {
		uint1* out = target.pixels();
		for (int4 y = y0; y < y1; y++) {
			for (int4 x = x0; x < x1; x++) {
				const int4 quadrant = (y >= kHalfTile ? 2 : 0) + (x >= kHalfTile ? 1 : 0);
				uint1* destination = out + ((ualni) y * kTileSize + (ualni) x) * 4;
				const Tile* source = quadrants[quadrant];
				if (!source) {
					destination[0] = destination[1] = destination[2] = destination[3] = 0;
					continue;
				}
				const int4 sx = (x % kHalfTile) * 2;
				const int4 sy = (y % kHalfTile) * 2;
				const uint1* row0 = source->pixels() + ((ualni) sy * kTileSize + (ualni) sx) * 4;
				const uint1* row1 = row0 + (ualni) kTileSize * 4;
				for (int4 channel = 0; channel < 4; channel++) {
					const int4 sum = row0[channel] + row0[channel + 4] + row1[channel] + row1[channel + 4];
					destination[channel] = (uint1) ((sum + 2) / 4);
				}
			}
		}
		target.markDirty({ (halnf) x0, (halnf) y0, (halnf) (x1 - x0), (halnf) (y1 - y0) });
	}

	template <typename Lookup>
	void LayerLevels::updateLevel(
		int4 level,
		const Lookup& children,
		const std::vector<TileCoord>& changedChildren,
		std::vector<TileCoord>& changedParents,
		ParallelFor* parallel
	) {
		Level& target = mLevels[(size_t) level - 1];
		Map<ualni, TileCoord> parents;
		Map<ualni, int4> quadrantMask;
		for (const TileCoord& child : changedChildren) {
			const TileCoord parent = parentOf(child);
			parents.put(parent.key(), parent);
			const int4 bit = 1 << ((child.y - parent.y * 2) * 2 + (child.x - parent.x * 2));
			auto found = quadrantMask.presents(parent.key());
			if (found.isValid()) quadrantMask.getSlotVal(found) |= bit;
			else quadrantMask.put(parent.key(), bit);
		}

		struct Job {
			const Tile* quadrants[4];
			Tile* target;
			int4 mask;
		};
		std::vector<Job> jobs;
		for (auto it : parents) {
			const TileCoord parent = it->val;
			const Tile* quadrants[4] = {
				children({ parent.x * 2, parent.y * 2 }),
				children({ parent.x * 2 + 1, parent.y * 2 }),
				children({ parent.x * 2, parent.y * 2 + 1 }),
				children({ parent.x * 2 + 1, parent.y * 2 + 1 }),
			};
			changedParents.push_back(parent);
			auto found = target.presents(parent.key());
			if (mPreview[level].presents(parent.key()).isValid()) continue;
			if (!quadrants[0] && !quadrants[1] && !quadrants[2] && !quadrants[3]) {
				if (found.isValid()) {
					mRemoved.push_back(target.getSlotVal(found).tile);
					target.remove(parent.key());
				}
				continue;
			}
			int4 mask = quadrantMask.get(parent.key());
			if (!found.isValid()) {
				target.put(parent.key(), { parent, std::make_shared<Tile>() });
				mask = 15;
			}
			jobs.push_back(
				{ { quadrants[0], quadrants[1], quadrants[2], quadrants[3] }, target.get(parent.key()).tile.get(), mask }
			);
			mLastDownsampled++;
			mRebuilds++;
			for (int4 quadrant = 0; quadrant < 4; quadrant++)
				if (mask & (1 << quadrant)) mLastPixels += (ualni) kHalfTile * kHalfTile;
		}

		const auto run = [&jobs](size_t index) {
			const Job& job = jobs[index];
			for (int4 quadrant = 0; quadrant < 4; quadrant++) {
				if (!(job.mask & (1 << quadrant))) continue;
				const int4 x0 = (quadrant & 1) * kHalfTile;
				const int4 y0 = (quadrant >> 1) * kHalfTile;
				downsample(job.quadrants, *job.target, x0, y0, x0 + kHalfTile, y0 + kHalfTile);
			}
		};
		if (parallel) parallel->run(jobs.size(), run);
		else
			for (size_t index = 0; index < jobs.size(); index++)
				run(index);
	}

	std::vector<TileCoord> LayerLevels::holdBackPending(std::vector<TileCoord> changed) {
		mPending.clear();
		for (int4 level = 1; level < kLevels; level++)
			mStale[level].removeAll();
		if (!mIsPending) return changed;

		Map<ualni, bool> heldParents;
		for (const TileCoord& coord : changed) {
			const TileCoord parent = parentOf(coord);
			if (heldParents.presents(parent.key()).isValid()) continue;
			if (anyChildPending(parent)) heldParents.put(parent.key(), true);
		}
		if (heldParents.size() == 0) return changed;

		std::vector<TileCoord> ready;
		for (const TileCoord& coord : changed) {
			if (heldParents.presents(parentOf(coord).key()).isValid()) mPending.push_back(coord);
			else ready.push_back(coord);
		}
		for (const TileCoord& coord : mPending)
			markStale(coord);
		return ready;
	}

	bool LayerLevels::anyChildPending(const TileCoord& parent) const {
		for (int4 child = 0; child < 4; child++)
			if (mIsPending({ parent.x * 2 + (child & 1), parent.y * 2 + (child >> 1) })) return true;
		return false;
	}

	void LayerLevels::markStale(const TileCoord& child) {
		TileCoord coord = child;
		for (int4 level = 1; level < kLevels; level++) {
			coord = parentOf(coord);
			mStale[level].put(coord.key(), coord);
		}
	}

	bool LayerLevels::isStale(int4 level, const TileCoord& coord) const {
		return level >= 1 && level < kLevels && mStale[level].presents(coord.key()).isValid();
	}

	ualni LayerLevels::staleCount() const {
		ualni count = 0;
		for (const Map<ualni, TileCoord>& stale : mStale)
			count += stale.size();
		return count;
	}

	void LayerLevels::update(const Layer& layer, int4 upToLevel, ParallelFor* parallel) {
		mLastDownsampled = 0;
		mLastPixels = 0;
		mRemoved.clear();
		for (std::vector<TileCoord>& changed : mChanged)
			changed.clear();
		upToLevel = std::clamp(upToLevel, 0, kLevels - 1);

		layer.forEachLevelChanged([this](const TileCoord& coord) { mPending.push_back(coord); });
		layer.clearLevelChanges();

		if (upToLevel <= 0 && mBuilt == 0) return;

		std::vector<TileCoord> changed;
		if (mBuilt == 0) {
			layer.forEachTile([&changed](const TileCoord& coord, Tile*) { changed.push_back(coord); });
			mPending.clear();
		} else {
			changed = holdBackPending(std::move(mPending));
		}
		mChanged[0] = changed;

		const int4 target = std::max(upToLevel, mBuilt);
		for (int4 level = 1; level <= target; level++) {
			if (level > mBuilt) {
				changed.clear();
				if (level == 1) layer.forEachTile([&changed](const TileCoord& coord, Tile*) { changed.push_back(coord); });
				else
					for (auto it : mLevels[(size_t) level - 2])
						changed.push_back(it->val.coord);
			}
			if (level <= mBuilt) {
				for (const TileCoord& released : mReleased[level])
					for (int4 child = 0; child < 4; child++)
						changed.push_back({ released.x * 2 + (child & 1), released.y * 2 + (child >> 1) });
			}
			mReleased[level].clear();
			std::vector<TileCoord> parents;
			if (level == 1) {
				updateLevel(
					level, [&layer](const TileCoord& coord) { return layer.tileIfExists(coord); }, changed, parents, parallel
				);
			} else {
				const Level& below = mLevels[(size_t) level - 2];
				updateLevel(
					level,
					[&below](const TileCoord& coord) -> Tile* {
						auto found = below.presents(coord.key());
						return found.isValid() ? below.getSlotVal(found).tile.get() : nullptr;
					},
					changed,
					parents,
					parallel
				);
			}
			mChanged[level] = parents;
			for (const TileCoord& parent : parents)
				mAccumulated[level].put(parent.key(), parent);
			changed = std::move(parents);
		}
		mBuilt = target;
	}

	Tile* LayerLevels::previewTile(int4 level, const TileCoord& coord) {
		if (level < 1 || level >= kLevels || level > mBuilt) return nullptr;
		Level& target = mLevels[(size_t) level - 1];
		auto found = target.presents(coord.key());
		if (!mPreview[level].presents(coord.key()).isValid()) {
			std::shared_ptr<Tile> original = found.isValid() ? target.getSlotVal(found).tile : nullptr;
			mOriginals[level].put(coord.key(), { coord, original });
			auto copy = std::make_shared<Tile>(Tile::Uninitialized{});
			if (original) std::memcpy(copy->pixels(), original->pixels(), (size_t) kTileSize * kTileSize * 4);
			else std::memset(copy->pixels(), 0, (size_t) kTileSize * kTileSize * 4);
			target.put(coord.key(), { coord, copy });
		}
		mPreview[level].put(coord.key(), coord);
		mAccumulated[level].put(coord.key(), coord);
		return target.get(coord.key()).tile.get();
	}

	Tile* LayerLevels::basePreviewTile(const Layer& layer, const TileCoord& coord) {
		auto found = mBase.presents(coord.key());
		if (found.isValid()) return mBase.getSlotVal(found).tile.get();
		auto tile = std::make_shared<Tile>(Tile::Uninitialized{});
		if (const Tile* source = layer.tileIfExists(coord))
			std::memcpy(tile->pixels(), source->pixels(), (size_t) kTileSize * kTileSize * 4);
		else std::memset(tile->pixels(), 0, (size_t) kTileSize * kTileSize * 4);
		mBase.put(coord.key(), { coord, tile });
		mPreview[0].put(coord.key(), coord);
		return tile.get();
	}

	Tile* LayerLevels::basePreview(const TileCoord& coord) const {
		auto found = mBase.presents(coord.key());
		return found.isValid() ? mBase.getSlotVal(found).tile.get() : nullptr;
	}

	bool LayerLevels::isPreview(int4 level, const TileCoord& coord) const {
		return level >= 0 && level < kLevels && mPreview[level].presents(coord.key()).isValid();
	}

	ualni LayerLevels::previewCount() const {
		ualni count = 0;
		for (const Map<ualni, TileCoord>& preview : mPreview)
			count += preview.size();
		return count;
	}

	void LayerLevels::discardPreview() {
		mBase.removeAll();
		mPreview[0].removeAll();
		for (int4 level = 1; level < kLevels; level++) {
			Level& target = mLevels[(size_t) level - 1];
			for (auto it : mOriginals[level]) {
				const LevelTile& original = it->val;
				if (original.tile) target.put(it->key, original);
				else if (target.presents(it->key).isValid()) target.remove(it->key);
				mAccumulated[level].put(it->key, original.coord);
			}
			mOriginals[level].removeAll();
			mPreview[level].removeAll();
		}
	}

	void LayerLevels::endPreview() {
		mBase.removeAll();
		mPreview[0].removeAll();
		for (int4 level = 1; level < kLevels; level++)
			mOriginals[level].removeAll();
		for (int4 level = 1; level < kLevels; level++) {
			for (auto it : mPreview[level])
				mReleased[level].push_back(it->val);
			mPreview[level].removeAll();
		}
	}

	Tile* LayerLevels::tileAt(int4 level, const TileCoord& coord) const {
		if (level < 1 || level > mBuilt) return nullptr;
		auto found = mLevels[(size_t) level - 1].presents(coord.key());
		return found.isValid() ? mLevels[(size_t) level - 1].getSlotVal(found).tile.get() : nullptr;
	}

	ualni LayerLevels::tileCount(int4 level) const {
		if (level < 1 || level > mBuilt) return 0;
		return mLevels[(size_t) level - 1].size();
	}

	const std::vector<TileCoord>& LayerLevels::lastChanged(int4 level) const {
		if (level < 0 || level >= kLevels) return kNoChanges;
		return mChanged[level];
	}
}
