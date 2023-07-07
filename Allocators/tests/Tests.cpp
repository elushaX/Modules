
#include "Allocators.hpp"

#include "Tests.hpp"

using namespace tp;

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleAllocators, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("AllocatorsTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testAll();

	testModule.deinitialize();
}
