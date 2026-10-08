#pragma once

#include "Brush.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace tp::sketch2d {

	struct DabMask {
		static constexpr int4 kOne = 1 << 14;

		int4 left = 0;
		int4 top = 0;
		int4 width = 0;
		int4 height = 0;
		std::vector<uint16_t> coverage;
		std::vector<uint16_t> distance;

		[[nodiscard]] bool isEmpty() const { return width <= 0 || height <= 0; }
		[[nodiscard]] size_t index(int4 x, int4 y) const {
			return (size_t) (y - top) * (size_t) width + (size_t) (x - left);
		}
	};

	struct DabPlacement {
		Vec2F center;
		int4 phaseX = 0;
		int4 phaseY = 0;
		int4 steps = 16;

		[[nodiscard]] static int4 stepsFor(halnf size);
		[[nodiscard]] static DabPlacement snap(Vec2F center, halnf size);
	};

	struct DabQuantizer {
		static constexpr int4 kSizeSteps = 512;
		static constexpr int4 kAngleSteps = 1024;
		static constexpr int4 kFractionSteps = 64;

		[[nodiscard]] static halnf size(halnf size);
		[[nodiscard]] static halnf angle(halnf degrees);
		[[nodiscard]] static halnf fraction(halnf value);
		[[nodiscard]] static DabShape shape(DabShape shape);
	};

	class DabMaskCache {
	public:
		static constexpr size_t kCapacity = 4096;

		[[nodiscard]] static DabMaskCache& shared();

		[[nodiscard]] std::shared_ptr<const DabMask>
		mask(const PreparedDab& dab, const DabPlacement& placement, halnf scale);
		[[nodiscard]] size_t size() const;
		[[nodiscard]] uint64_t hits() const { return mHits; }
		[[nodiscard]] uint64_t misses() const { return mMisses; }
		void clear();

		[[nodiscard]] static std::shared_ptr<DabMask>
		rasterize(const PreparedDab& dab, const DabPlacement& placement, halnf scale);

	private:
		[[nodiscard]] static uint64_t keyOf(const DabShape& shape, const DabPlacement& placement, halnf scale);

		mutable std::mutex mMutex;
		std::unordered_map<uint64_t, std::shared_ptr<const DabMask>> mMasks;
		uint64_t mHits = 0;
		uint64_t mMisses = 0;
	};
}
