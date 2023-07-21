
#include "ConnectionCommon.hpp"
#include "Testing.hpp"

void testLocalConnection();

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleConnection, nullptr };
	tp::ModuleManifest testModule("ConnectionTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testLocalConnection();

	testModule.deinitialize();
}