#include "StrokePainter.hpp"
#include "Layer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace tp::sketch2d {

	namespace {
		constexpr size_t kSubmitBatch = 256;

		uint1 toByte(halnf value) {
			if (value <= 0.0f) return 0;
			if (value >= 255.0f) return 255;
			return (uint1) (int4) (value + 0.5f);
		}

		void blendMarker(uint1* premultiplied, const halnf source[3], halnf coverage) {
			const halnf alpha = premultiplied[3] / 255.0f;
			const halnf overAlpha = alpha + (1.0f - alpha) * coverage;
			const halnf keptAlpha = std::max(alpha, coverage);

			for (int4 channel = 0; channel < 3; channel++) {
				const halnf mixed = premultiplied[channel] + (source[channel] - premultiplied[channel]) * coverage;
				const halnf straight = overAlpha > 0.0f ? mixed / overAlpha : 0.0f;
				premultiplied[channel] = toByte(straight * keptAlpha);
			}
			premultiplied[3] = toByte(255.0f * keptAlpha);
		}

		int64_t nowNs() {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
				.count();
		}
	}

	StrokePainter::~StrokePainter() { flush(); }

	void StrokePainter::setWorkers(size_t count) {
		flush();
		mLanes.reset();
		mStaged.clear();
		mNotes.clear();
		if (count == 0) return;

		mStaged.resize(count);
		mNotes.resize(count);
		mLanes = std::make_unique<WorkerLanes<DabJob>>(count, [this](size_t lane, DabJob& job) { run(lane, job); });
	}

	size_t StrokePainter::laneOf(TileCoord coord, int4 bandY) const {
		const ualni key = coord.key() * 0x9e3779b97f4a7c15ull + (ualni) (bandY / kBandRows);
		return (size_t) ((key ^ (key >> 29)) % mStaged.size());
	}

	void StrokePainter::stage(
		const std::shared_ptr<const DabWork>& work,
		Tile& tile,
		Layer& layer,
		TileCoord coord,
		int4 x0,
		int4 y0,
		int4 x1,
		int4 y1,
		StrokeMask* mask
	) {
		if (!mLanes) {
			if (mask) paintMasked(*work, *mask, tile, coord, x0, y0, x1, y1);
			else paint(*work, tile, coord, x0, y0, x1, y1);
			tile.markDirty({ (halnf) x0, (halnf) y0, (halnf) (x1 - x0), (halnf) (y1 - y0) });
			return;
		}

		for (int4 band = y0; band < y1;) {
			const int4 bandEnd = std::min(y1, (band / kBandRows + 1) * kBandRows);
			mStaged[laneOf(coord, band)].push_back({ work, &tile, mask, &layer, coord, x0, band, x1, bandEnd });
			mStagedCount++;
			band = bandEnd;
		}
		if (mStagedCount >= kSubmitBatch) submit();
	}

	void StrokePainter::submit() {
		if (!mLanes || mStagedCount == 0) return;
		for (size_t lane = 0; lane < mStaged.size(); lane++)
			mLanes->pushAll(lane, mStaged[lane]);
		mStagedCount = 0;
	}

	void StrokePainter::flush() {
		if (!mLanes) return;
		if (isPausedHere()) {
			for (size_t lane = 0; lane < mStaged.size(); lane++) {
				for (DabJob& job : mStaged[lane])
					run(lane, job);
				mStaged[lane].clear();
			}
			mStagedCount = 0;
			mLanes->runQueuedHere();
			applyNotes();
			return;
		}
		submit();
		mLanes->wait();
		applyNotes();
	}

	void StrokePainter::applyNotes() {
		for (std::vector<DirtyNote>& notes : mNotes) {
			for (const DirtyNote& note : notes) {
				note.tile->markDirty(
					{ (halnf) note.x0, (halnf) note.y0, (halnf) (note.x1 - note.x0), (halnf) (note.y1 - note.y0) }
				);
				note.layer->noteChanged(note.coord);
			}
			notes.clear();
		}
	}

	double StrokePainter::takeWorkerMs() { return (double) mWorkerNs.exchange(0) / 1.0e6; }

	void StrokePainter::run(size_t lane, DabJob& job) {
		const int64_t start = nowNs();
		if (job.mask) paintMasked(*job.work, *job.mask, *job.tile, job.coord, job.x0, job.y0, job.x1, job.y1);
		else paint(*job.work, *job.tile, job.coord, job.x0, job.y0, job.x1, job.y1);
		std::vector<DirtyNote>& notes = mNotes[lane];
		if (!notes.empty() && notes.back().tile == job.tile && notes.back().y0 / kBandRows == job.y0 / kBandRows) {
			DirtyNote& last = notes.back();
			last.x0 = std::min(last.x0, job.x0);
			last.y0 = std::min(last.y0, job.y0);
			last.x1 = std::max(last.x1, job.x1);
			last.y1 = std::max(last.y1, job.y1);
		} else {
			notes.push_back({ job.tile, job.layer, job.coord, job.x0, job.y0, job.x1, job.y1 });
		}
		mWorkerNs.fetch_add(nowNs() - start, std::memory_order_relaxed);
	}

	namespace {
		struct DabPixel {
			int4 x;
			int4 y;
			halnf coverage;
		};

		struct Blend {
			const DabWork& work;
			uint1* pixels;
			StrokeMask* mask;
			const uint1* base;

			void apply(int4 x, int4 y, halnf a) const {
				const size_t index = (size_t) y * kTileSize + (size_t) x;
				uint1* dst = pixels + index * 4;
				const halnf* source = work.source;
				if (mask) {
					uint16_t* coverage = mask->coverage();
					coverage[index] = StrokeMask::raise(coverage[index], a);
					const halnf stroke = (halnf) coverage[index] / (halnf) StrokeMask::kOne * work.opacity;
					const halnf below = 1.0f - stroke;
					const uint1* under = base ? base + index * 4 : nullptr;
					for (int4 channel = 0; channel < 3; channel++)
						dst[channel] = toByte(source[channel] * stroke + (under ? under[channel] : 0.0f) * below);
					dst[3] = toByte(255.0f * stroke + (under ? under[3] : 0.0f) * below);
					return;
				}
				const halnf keep = 1.0f - a;
				if (work.erase) {
					for (int4 channel = 0; channel < 4; channel++)
						dst[channel] = toByte(dst[channel] * keep);
				} else if (work.marker && !work.keepAlpha) {
					blendMarker(dst, source, a);
				} else if (work.keepAlpha) {
					const halnf alpha = dst[3] / 255.0f;
					for (int4 channel = 0; channel < 3; channel++)
						dst[channel] = toByte(source[channel] * alpha * a + dst[channel] * keep);
				} else {
					for (int4 channel = 0; channel < 3; channel++)
						dst[channel] = toByte(source[channel] * a + dst[channel] * keep);
					dst[3] = toByte(255.0f * a + dst[3] * keep);
				}
			}
		};

		halnf finishCoverage(const DabWork& work, halnf a, halnf distance, Vec2F world, bool masked) {
			if (!work.dab.plain()) {
				a = work.dab.shape().textured(a, world, distance);
				if (a <= 0.0f) return 0;
			}
			a *= masked ? work.strength : work.strength * work.opacity;
			if (work.selection) a *= work.selection->coverage((int4) world.x, (int4) world.y) / 255.0f;
			return a;
		}
	}

	void StrokePainter::paintAtLevel(
		const DabWork& work, int4 level, Tile& tile, TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1, StrokeMask* mask
	) {
		const halnf scale = (halnf) (1 << std::max(0, level));
		const DabPlacement placement = DabPlacement::snap(work.center / scale, work.dab.shape().size / scale);
		const Blend blend{ work, tile.pixels(), mask, mask && mask->base() ? mask->base()->pixels() : nullptr };
		const int4 originX = coord.x * kTileSize;
		const int4 originY = coord.y * kTileSize;
		const int4 anchorX = (int4) std::floor(placement.center.x) - originX;
		const int4 anchorY = (int4) std::floor(placement.center.y) - originY;

		if (std::shared_ptr<const DabMask> cached = DabMaskCache::shared().mask(work.dab, placement, scale)) {
			const DabMask& dab = *cached;
			const int4 fromY = std::max(y0, anchorY + dab.top);
			const int4 toY = std::min(y1, anchorY + dab.top + dab.height);
			const int4 fromX = std::max(x0, anchorX + dab.left);
			const int4 toX = std::min(x1, anchorX + dab.left + dab.width);
			for (int4 y = fromY; y < toY; y++) {
				for (int4 x = fromX; x < toX; x++) {
					const size_t index = dab.index(x - anchorX, y - anchorY);
					const uint16_t value = dab.coverage[index];
					if (value == 0) continue;
					const Vec2F world = { ((halnf) (originX + x) + 0.5f) * scale, ((halnf) (originY + y) + 0.5f) * scale };
					const halnf a = finishCoverage(
						work,
						(halnf) value / (halnf) DabMask::kOne,
						(halnf) dab.distance[index] / (halnf) DabMask::kOne,
						world,
						mask != nullptr
					);
					if (a > 0.0f) blend.apply(x, y, a);
				}
			}
			return;
		}

		const PreparedDab& dab = work.dab;
		const halnf inverse = 1.0f / scale;
		const halnf radius = dab.shape().radius() * inverse + 1.0f;
		const halnf centerX = placement.center.x - (halnf) originX;
		const halnf centerY = placement.center.y - (halnf) originY;
		const int4 fromY = std::max(y0, (int4) std::floor(centerY - radius));
		const int4 toY = std::min(y1, (int4) std::ceil(centerY + radius) + 1);
		for (int4 y = fromY; y < toY; y++) {
			const halnf wy = ((halnf) y + 0.5f - centerY) * scale;
			halnf spanFrom = 0;
			halnf spanTo = 0;
			if (!dab.rowSpan(wy, spanFrom, spanTo)) continue;
			const int4 fromX = std::max(x0, (int4) std::floor(centerX - 0.5f + spanFrom * inverse) - 1);
			const int4 toX = std::min(x1, (int4) std::ceil(centerX - 0.5f + spanTo * inverse) + 2);
			for (int4 x = fromX; x < toX; x++) {
				const halnf wx = ((halnf) x + 0.5f - centerX) * scale;
				halnf distance = 0;
				const halnf value = dab.coverage(wx, wy, distance);
				if (value <= 0.0f) continue;
				const Vec2F world = { ((halnf) (originX + x) + 0.5f) * scale, ((halnf) (originY + y) + 0.5f) * scale };
				const halnf a = finishCoverage(work, value, distance, world, mask != nullptr);
				if (a > 0.0f) blend.apply(x, y, a);
			}
		}
	}

	void StrokePainter::paint(const DabWork& work, Tile& tile, TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1) {
		paintAtLevel(work, 0, tile, coord, x0, y0, x1, y1, nullptr);
	}

	void StrokePainter::paintMasked(
		const DabWork& work, StrokeMask& mask, Tile& tile, TileCoord coord, int4 x0, int4 y0, int4 x1, int4 y1
	) {
		paintAtLevel(work, 0, tile, coord, x0, y0, x1, y1, &mask);
	}
}
