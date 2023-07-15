
#include "Tests.hpp"
#include "Testing.hpp"


int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, &tp::gModuleTokenizer, nullptr };
	tp::ModuleManifest testModule("CommandLineTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testCommandLine();

	testModule.deinitialize();
}
