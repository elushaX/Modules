
#include "Testing.hpp"
#include "Utils.hpp"

void testLocalStorage();

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("StorageTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testLocalStorage();

	testModule.deinitialize();
}