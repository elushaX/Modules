
#include "NewPlacement.hpp"

#include "FCNN.hpp"
#include "Testing.hpp"
#include "Utils.hpp"

#include <cstdio>

using namespace tp;

void test() {
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
	halnf steppingValue = 100;

	for (auto i : Range(50)) {

		nn.evaluate(input, output);
		
		nn.calcGrad(outputExpected);
		nn.applyGrad(steppingValue);

		printf("Loss %f \n", nn.calcCost(outputExpected));
	}
}

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleDataAnalysis, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("DataAnalysisTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	test();

	testModule.deinitialize();

	return 0;
}
