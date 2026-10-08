#include "Noise.hpp"

#include <cmath>
#include <numeric>

namespace tp {

	namespace {
		constexpr halnf kGradients[8][2] = {
			{ 1, 0 },
			{ -1, 0 },
			{ 0, 1 },
			{ 0, -1 },
			{ 0.7071f, 0.7071f },
			{ -0.7071f, 0.7071f },
			{ 0.7071f, -0.7071f },
			{ -0.7071f, -0.7071f },
		};

		halnf fade(halnf t) { return t * t * t * (t * (t * 6 - 15) + 10); }

		int4 wrap(int4 value, int4 period) {
			const int4 remainder = value % period;
			return remainder < 0 ? remainder + period : remainder;
		}

		halnf dotGradient(int4 hash, halnf x, halnf y) {
			const halnf* gradient = kGradients[hash & 7];
			return gradient[0] * x + gradient[1] * y;
		}
	}

	TileableNoise::TileableNoise(uint32_t seed) {
		std::array<uint8_t, 256> order{};
		std::iota(order.begin(), order.end(), 0);
		uint32_t state = seed * 747796405u + 2891336453u;
		for (int4 index = 255; index > 0; index--) {
			state = state * 1664525u + 1013904223u;
			const int4 other = (int4) ((state >> 8) % (uint32_t) (index + 1));
			std::swap(order[index], order[other]);
		}
		for (int4 index = 0; index < 512; index++)
			mPermutation[index] = order[index & 255];
	}

	int4 TileableNoise::hash(int4 x, int4 y) const { return mPermutation[mPermutation[x & 255] + (y & 255)]; }

	halnf TileableNoise::gradient(halnf x, halnf y, int4 periodX, int4 periodY) const {
		const halnf floorX = std::floor(x);
		const halnf floorY = std::floor(y);
		const int4 x0 = wrap((int4) floorX, periodX);
		const int4 y0 = wrap((int4) floorY, periodY);
		const int4 x1 = wrap(x0 + 1, periodX);
		const int4 y1 = wrap(y0 + 1, periodY);
		const halnf fx = x - floorX;
		const halnf fy = y - floorY;

		const halnf n00 = dotGradient(hash(x0, y0), fx, fy);
		const halnf n10 = dotGradient(hash(x1, y0), fx - 1, fy);
		const halnf n01 = dotGradient(hash(x0, y1), fx, fy - 1);
		const halnf n11 = dotGradient(hash(x1, y1), fx - 1, fy - 1);

		const halnf u = fade(fx);
		const halnf v = fade(fy);
		const halnf top = n00 + (n10 - n00) * u;
		const halnf bottom = n01 + (n11 - n01) * u;
		return (top + (bottom - top) * v) * 1.4142f;
	}

	halnf TileableNoise::fractal(halnf x, halnf y, int4 period, const halnf* weights, int4 octaves) const {
		halnf sum = 0;
		halnf total = 0;
		int4 scale = 1;
		for (int4 octave = 0; octave < octaves; octave++) {
			sum += gradient(x * (halnf) scale, y * (halnf) scale, period * scale, period * scale) * weights[octave];
			total += weights[octave];
			scale *= 2;
		}
		return total > 0 ? sum / total : 0;
	}
}
