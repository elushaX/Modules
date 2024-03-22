
#include "UnitTest++/UnitTest++.h"

#include "FCNN.hpp"
#include "Utils.hpp"

#include <cstdio>

using namespace tp;

halnf inputLayer[100] = {
	6.50073, 2.41955, 3.57390, 2.54054, 4.01545, 1.27555, 8.93310, 5.98754, 0.83661, 2.30855, 7.70906, 6.53513, 5.92166,
	9.96563, 1.24706, 3.17029, 8.52388, 8.75402, 8.73118, 0.56217, 1.02163, 7.27230, 5.51980, 2.10261, 9.94351, 1.89839,
	4.23003, 3.80691, 9.43242, 2.74919, 0.30270, 9.76303, 5.72574, 7.80288, 8.62568, 6.25799, 0.03797, 2.56737, 5.57602,
	9.85272, 8.81618, 6.40643, 6.51678, 6.28869, 5.20166, 2.39731, 2.18484, 0.61359, 6.27063, 1.89839, 1.38699, 2.49058,
	7.03768, 9.24257, 7.14492, 9.79083, 7.09768, 6.10761, 2.06128, 5.79821, 9.65542, 6.98304, 3.00416, 9.48536, 2.21974,
	4.31504, 0.70457, 4.78254, 1.76592, 6.04608, 2.58798, 4.91384, 8.95884, 8.09472, 7.13455, 9.15459, 1.94269, 5.99912,
	6.79787, 1.03997, 5.23773, 9.31124, 9.44900, 8.06692, 4.86936, 7.97706, 7.48052, 4.77991, 9.32074, 2.11929, 8.23583,
	2.80812, 1.94275, 0.42090, 6.15178, 2.26695, 4.12673, 5.84851, 0.22800, 3.11190
};

halnf outputLayer[20] = {
	1.000000, 0.283636, 0.725544, 1.000000, 0.921875, 0.131610, 0.276638, 0.894206, 0.379333, 0.601731,
	0.074395, 0.598880, 0.454188, 0.424037, 0.961471, 0.930909, 0.681503, 0.734448, 0.123868, 1.000000
};

SUITE(FCNN) {
	TEST(Basic) {
		Buffer<halni> layers = { 100, 70, 50, 30, 20 };
		Buffer<halnf> input(layers.first());
		Buffer<halnf> outputExpected(layers.last());
		Buffer<halnf> output(layers.last());

		for (auto inputVal : Range(layers.first())) {
			input[inputVal] = inputLayer[inputVal];
		}

		for (auto outIdx : Range(layers.last())) {
			outputExpected[outIdx] = outputLayer[outIdx];
		}

		FCNN nn(layers);
		halnf steppingValue = 0.1;

		halnf cost = 0;

		for (auto i : Range(150)) {

			nn.evaluate(input, output);
		
			nn.calcGrad(outputExpected);
			nn.applyGrad(steppingValue);

			cost = nn.calcCost(outputExpected);
			// printf("Loss %f \n", nn.calcCost(outputExpected));
		}

		CHECK(cost < 0.15f);
	}
}

int main() {
	return UnitTest::RunAllTests();
}
