#include "DabMask.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace tp::sketch2d {

	namespace {
		constexpr int4 kLargestCachedSide = 256;
		constexpr int4 kAreaSamples = 4;

		uint64_t mix(uint64_t hash, uint64_t value) {
			hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
			return hash;
		}

		uint64_t bitsOf(halnf value) {
			uint32_t bits = 0;
			std::memcpy(&bits, &value, sizeof bits);
			return bits;
		}

		uint16_t toFixed(halnf value) {
			return (uint16_t) std::lround(std::clamp(value, 0.0f, 1.0f) * (halnf) DabMask::kOne);
		}
	}

	namespace {
		struct SizeTable {
			std::array<halnf, DabQuantizer::kSizeSteps> values{};

			SizeTable() {
				const std::pair<int4, halnf> runs[] = {
					{ 32, 0.25f }, { 64, 0.5f }, { 96, 1.0f }, { 128, 2.0f }, { 192, 4.0f }
				};
				halnf value = 0;
				int4 index = 0;
				for (const auto& [count, step] : runs)
					for (int4 run = 0; run < count; run++)
						values[(size_t) index++] = value += step;
			}
		};

		const SizeTable& sizeTable() {
			static const SizeTable table;
			return table;
		}
	}

	halnf DabQuantizer::size(halnf size) {
		const auto& values = sizeTable().values;
		if (size <= values.front()) return values.front();
		if (size >= values.back()) return size;
		const auto upper = std::lower_bound(values.begin(), values.end(), size);
		const auto lower = upper - 1;
		return size - *lower < *upper - size ? *lower : *upper;
	}

	halnf DabQuantizer::angle(halnf degrees) {
		const halnf turn = degrees / 360.0f;
		const halnf wrapped = turn - std::floor(turn);
		const int4 step = (int4) std::lround(wrapped * (halnf) kAngleSteps) % kAngleSteps;
		return (halnf) step * 360.0f / (halnf) kAngleSteps;
	}

	halnf DabQuantizer::fraction(halnf value) {
		return std::round(std::clamp(value, 0.0f, 1.0f) * (halnf) kFractionSteps) / (halnf) kFractionSteps;
	}

	DabShape DabQuantizer::shape(DabShape shape) {
		const halnf across = shape.size * shape.roundness;
		shape.size = size(shape.size);
		if (shape.roundness < 1.0f && shape.size > 0.0f) shape.roundness = std::min(1.0f, size(across) / shape.size);
		shape.angleDegrees = angle(shape.angleDegrees);
		shape.interior = fraction(shape.interior);
		shape.hardness = fraction(shape.hardness);
		return shape;
	}

	int4 DabPlacement::stepsFor(halnf size) {
		if (size > 256.0f) return 2;
		if (size > 64.0f) return 4;
		if (size > 16.0f) return 8;
		return 16;
	}

	DabPlacement DabPlacement::snap(Vec2F center, halnf size) {
		DabPlacement placement;
		placement.steps = stepsFor(size);
		const halnf steps = (halnf) placement.steps;
		const int4 x = (int4) std::lround(center.x * steps);
		const int4 y = (int4) std::lround(center.y * steps);
		placement.center = { (halnf) x / steps, (halnf) y / steps };
		placement.phaseX = ((x % placement.steps) + placement.steps) % placement.steps;
		placement.phaseY = ((y % placement.steps) + placement.steps) % placement.steps;
		return placement;
	}

	DabMaskCache& DabMaskCache::shared() {
		static DabMaskCache cache;
		return cache;
	}

	uint64_t DabMaskCache::keyOf(const DabShape& shape, const DabPlacement& placement, halnf scale) {
		uint64_t hash = 0;
		for (const halnf value : { shape.size, shape.interior, shape.hardness, shape.roundness, shape.angleDegrees, scale })
			hash = mix(hash, bitsOf(value));
		hash = mix(hash, (uint64_t) placement.steps);
		hash = mix(hash, (uint64_t) placement.phaseX);
		return mix(hash, (uint64_t) placement.phaseY);
	}

	std::shared_ptr<DabMask> DabMaskCache::rasterize(const PreparedDab& dab, const DabPlacement& placement, halnf scale) {
		auto mask = std::make_shared<DabMask>();
		if (dab.isEmpty()) return mask;
		const halnf inverse = 1.0f / scale;
		const halnf fractionX = (halnf) placement.phaseX / (halnf) placement.steps;
		const halnf fractionY = (halnf) placement.phaseY / (halnf) placement.steps;
		const int4 reach = (int4) std::ceil(dab.shape().radius() * inverse) + 1;
		mask->left = -reach;
		mask->top = -reach;
		mask->width = reach * 2 + 1;
		mask->height = reach * 2 + 1;
		const size_t area = (size_t) mask->width * (size_t) mask->height;
		mask->coverage.assign(area, 0);
		mask->distance.assign(area, 0);
		const int4 samples = scale > 1.0f ? kAreaSamples : 1;
		const halnf weight = 1.0f / (halnf) (samples * samples);
		for (int4 y = mask->top; y < mask->top + mask->height; y++) {
			for (int4 x = mask->left; x < mask->left + mask->width; x++) {
				halnf total = 0;
				halnf nearest = 1;
				for (int4 sy = 0; sy < samples; sy++) {
					const halnf wy = ((halnf) y + ((halnf) sy + 0.5f) / (halnf) samples - fractionY) * scale;
					for (int4 sx = 0; sx < samples; sx++) {
						const halnf wx = ((halnf) x + ((halnf) sx + 0.5f) / (halnf) samples - fractionX) * scale;
						halnf distance = 0;
						const halnf value = dab.coverage(wx, wy, distance);
						if (value <= 0.0f) continue;
						total += value;
						nearest = std::min(nearest, distance);
					}
				}
				if (total <= 0.0f) continue;
				const size_t index = mask->index(x, y);
				mask->coverage[index] = std::max<uint16_t>(1, toFixed(total * weight));
				mask->distance[index] = toFixed(nearest);
			}
		}
		return mask;
	}

	std::shared_ptr<const DabMask>
	DabMaskCache::mask(const PreparedDab& dab, const DabPlacement& placement, halnf scale) {
		const int4 side = (int4) std::ceil(dab.shape().size / scale) + 3;
		if (side > kLargestCachedSide) return nullptr;
		const uint64_t key = keyOf(dab.shape(), placement, scale);
		{
			std::lock_guard lock(mMutex);
			auto found = mMasks.find(key);
			if (found != mMasks.end()) {
				mHits++;
				return found->second;
			}
		}
		std::shared_ptr<const DabMask> created = rasterize(dab, placement, scale);
		std::lock_guard lock(mMutex);
		mMisses++;
		if (mMasks.size() >= kCapacity) mMasks.clear();
		return mMasks.emplace(key, std::move(created)).first->second;
	}

	size_t DabMaskCache::size() const {
		std::lock_guard lock(mMutex);
		return mMasks.size();
	}

	void DabMaskCache::clear() {
		std::lock_guard lock(mMutex);
		mMasks.clear();
		mHits = 0;
		mMisses = 0;
	}
}
