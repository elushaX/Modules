#pragma once

#include "Environment.hpp"

#include <array>
#include <cstdint>

namespace tp {

	class TileableNoise {
	public:
		explicit TileableNoise(uint32_t seed = 0);

		[[nodiscard]] halnf gradient(halnf x, halnf y, int4 periodX, int4 periodY) const;
		[[nodiscard]] halnf fractal(halnf x, halnf y, int4 period, const halnf* weights, int4 octaves) const;

	private:
		[[nodiscard]] int4 hash(int4 x, int4 y) const;

		std::array<uint8_t, 512> mPermutation{};
	};
}
