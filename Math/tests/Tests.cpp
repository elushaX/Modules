
#include "MathCommon.hpp"
#include "Testing.hpp"

static bool init(const tp::ModuleManifest* self) {
	tp::gTesting.setRootName(self->getName());
	return true;
}

void testMath();

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleMath, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("MathTest", init, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testMath();

	testModule.deinitialize();
}
