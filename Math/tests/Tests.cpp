
#include "ComplexNumbers.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;

SUITE(Math) {
	TEST(ComplexNumber) {
		CHECK((ComplexCart{ 3, 4 }.mod() == 5));
		CHECK((abs(ComplexCart{ 1, 1 }.arg() - 0.7853981633974483) < 0.001));
		CHECK((ComplexCart{ 3, 4 }.norm2() == 25));

		{
			ComplexCart cn{ 3, 4 };
			ComplexCart expected{ 0.12, -0.16 };
			auto val = cn.reciprocal();
			CHECK((val == expected));
		}

		{
			ComplexCart cn{ 3, 4 };
			ComplexCart expected{ 3, -4 };
			CHECK((cn.conjugate() == expected));
		}

		{
			ComplexCart cn1{ 3, 4 };
			ComplexCart cn2{ 1, 2 };
			ComplexCart expected{ -5, 10 };
			CHECK((cn1 * cn2 == expected));
		}

		{
			ComplexRad c1{ 5, 7 };
			ComplexRad c2{ 10, 3 };
			ComplexCart tmp1 = (c1 * c2).cart();
			ComplexCart tmp2 = (c1.cart() * c2.cart());
			CHECK((tmp1 - tmp2).mod() < 0.001);
		}
	}
}

int main() { return UnitTest::RunAllTests(); }

#include "Color.hpp"
#include "Noise.hpp"

#include <cmath>

SUITE(TileableNoise) {
	TEST(NoiseTilesAcrossItsPeriod) {
		const TileableNoise noise(7);
		for (int index = 0; index < 20; index++) {
			const halnf x = 0.37f * (halnf) index;
			const halnf y = 1.13f + 0.21f * (halnf) index;
			CHECK_CLOSE(noise.gradient(x, y, 8, 4), noise.gradient(x + 8, y, 8, 4), 1e-5f);
			CHECK_CLOSE(noise.gradient(x, y, 8, 4), noise.gradient(x, y + 4, 8, 4), 1e-5f);
		}
	}

	TEST(NoiseIsZeroOnLatticeAndBounded) {
		const TileableNoise noise(3);
		CHECK_CLOSE(0.0f, noise.gradient(2, 5, 16, 16), 1e-6f);
		halnf low = 0;
		halnf high = 0;
		for (int y = 0; y < 64; y++) {
			for (int x = 0; x < 64; x++) {
				const halnf value = noise.gradient((halnf) x * 0.13f, (halnf) y * 0.17f, 16, 16);
				low = std::min(low, value);
				high = std::max(high, value);
			}
		}
		CHECK(low >= -1.0f && high <= 1.0f);
		CHECK(high - low > 0.8f);
	}

	TEST(SeedsChangeThePattern) {
		const TileableNoise a(1);
		const TileableNoise b(2);
		halnf difference = 0;
		for (int index = 0; index < 32; index++)
			difference += std::fabs(a.gradient(0.3f * index, 0.7f, 32, 32) - b.gradient(0.3f * index, 0.7f, 32, 32));
		CHECK(difference > 1);
	}

	TEST(FractalStaysTileableAtEveryOctave) {
		const TileableNoise noise(5);
		const halnf weights[] = { 1, 0.5f, 0.25f, 0.125f };
		CHECK_CLOSE(noise.fractal(0.4f, 0.6f, 4, weights, 4), noise.fractal(4.4f, 4.6f, 4, weights, 4), 1e-5f);
	}
}

SUITE(DisplayGamma) {
	TEST(GammaRoundTripsAndMatchesReferenceTable) {
		CHECK_CLOSE(169.0f / 16384.0f, gammaToLinear(0.125f), 2e-4f);
		CHECK_CLOSE(6367.0f / 16384.0f, linearToGamma(0.125f), 2e-4f);
		for (halnf value = 0; value <= 1.0f; value += 0.05f)
			CHECK_CLOSE(value, linearToGamma(gammaToLinear(value)), 1e-5f);
		CHECK_EQUAL(0.0f, gammaToLinear(-1));
		CHECK_EQUAL(1.0f, linearToGamma(2));
	}
}
