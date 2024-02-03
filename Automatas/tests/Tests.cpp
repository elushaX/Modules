#include "AutomatasCommon.hpp"
#include "Utils.hpp"

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleAutomatas, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("Test", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testModule.deinitialize();
}
