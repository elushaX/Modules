
#include "Tests.hpp"
#include "Testing.hpp"

static bool init(const tp::ModuleManifest* self) {
	tp::gTesting.setRootName(self->getName());
	return true;
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, &tp::gModuleAllocators, nullptr };
	tp::ModuleManifest testModule("ContainersTest", init, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testIntervalTree();
	testList();
	testMap();
	testAvl();
	testBuffer();
	testBuffer2d();

	testModule.deinitialize();
}
