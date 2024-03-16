
#include "UnitTest++/UnitTest++.h"

#include "FCNN.hpp"
#include "Utils.hpp"

#include <cstdio>

using namespace tp;

SUITE(FCNN) {
	TEST(Basic) {
		Buffer<halni> layers = { 100, 70, 50, 30, 20 };
		Buffer<halnf> input(layers.first());
		Buffer<halnf> outputExpected(layers.last());
		Buffer<halnf> output(layers.last());

		for (auto inputVal : Range(layers.first())) {
			input[inputVal] = (halnf) randomFloat() * 100;
		}

		for (auto outIdx : Range(layers.last())) {
			outputExpected[outIdx] = (halnf) randomFloat();
		}

		FCNN nn(layers);
		halnf steppingValue = 0.01;

		halnf cost = 0;

		for (auto i : Range(150)) {

			nn.evaluate(input, output);
		
			nn.calcGrad(outputExpected);
			nn.applyGrad(steppingValue);

			cost = nn.calcCost(outputExpected);
			printf("Loss %f \n", nn.calcCost(outputExpected));
		}

		CHECK(cost < 0.1f);
	}
}

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleDataAnalysis, nullptr };
	tp::ModuleManifest testModule("DataAnalysisTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	bool res = UnitTest::RunAllTests();

	testModule.deinitialize();

	return res;
}
