
#include "FullyConnectedNN.hpp"
#include "Testing.hpp"
#include "Utils.hpp"

static bool init(const tp::ModuleManifest* self) {
	tp::gTesting.setRootName(self->getName());
	return true;
}

void test() {}

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
