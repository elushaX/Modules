
#include "Tests.hpp"
#include "UnitTest++/UnitTest++.h"

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, &tp::gModuleAllocators, nullptr };
	tp::ModuleManifest testModule("ContainersTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	bool res = UnitTest::RunAllTests();

	testModule.deinitialize();

	return res;
}
