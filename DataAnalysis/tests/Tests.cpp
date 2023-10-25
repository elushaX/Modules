
#include "NewPlacement.hpp"

#include "FullyConnectedNN.hpp"
#include "Testing.hpp"
#include "Utils.hpp"

#include <stdio.h>

static bool init(const tp::ModuleManifest* self) {
	tp::gTesting.setRootName(self->getName());
	return true;
}

void test() {
	using namespace tp;

	Buffer<halni> layers = { 4, 4, 3, 2 };
	Buffer<halnf> input = { (halnf) randomFloat() * 100, (halnf) randomFloat() * 100, (halnf) randomFloat() * 100, (halnf) randomFloat() * 100 };
	Buffer<halnf> outputExpected = { (halnf) randomFloat(), (halnf) randomFloat() };

	FullyConnectedNN nn;
	Buffer<halnf> output(2);

	nn.initializeRandom(layers);

	// nn.mLayers.last().neurons.first().weights = { 0.35, 0.35, 0.35, 0.35 };
	// nn.mLayers.last().neurons.first().bias = 0.9;

	// nn.mLayers.last().neurons.last().weights = { -0.35, -0.35, -0.35, -0.35 };
	// nn.mLayers.last().neurons.last().bias = -3.9;

	for (auto i : Range(50)) {
		nn.evaluate(input, output);

		auto lossBefore = nn.calcCost(outputExpected);

		nn.calcGrad(outputExpected);
		nn.applyGrad(0.1);
		printf("Loss %f \n", lossBefore);
	}
}

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleDataAnalysis, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("DataAnalysisTest", init, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	test();

	testModule.deinitialize();

	return 0;
}
