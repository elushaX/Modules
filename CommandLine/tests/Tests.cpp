#include "NewPlacement.hpp"

#include "Tests.hpp"

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, &tp::gModuleTokenizer, nullptr };
	tp::ModuleManifest testModule("CommandLineTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	// testCommandLine();
	testInterpreter();

	testModule.deinitialize();
}
