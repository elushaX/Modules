#pragma once

#include "Tile.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace tp::sketch2d {

	class StrokeMask {
	public:
		static constexpr int4 kOne = 1 << 14;

		StrokeMask() :
			mCoverage((size_t) kTileSize * kTileSize, 0) {}

		[[nodiscard]] uint16_t* coverage() { return mCoverage.data(); }
		[[nodiscard]] const uint16_t* coverage() const { return mCoverage.data(); }

		[[nodiscard]] const std::shared_ptr<const Tile>& base() const { return mBase; }
		void setBase(std::shared_ptr<const Tile> base) { mBase = std::move(base); }

		static uint16_t raise(uint16_t current, halnf amount) {
			const int4 strength = (int4) (amount * (halnf) kOne + 0.5f);
			return (uint16_t) (current + (((kOne - (int4) current) * strength) >> 14));
		}

	private:
		std::vector<uint16_t> mCoverage;
		std::shared_ptr<const Tile> mBase;
	};
}
