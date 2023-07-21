
#include "Testing.hpp"
#include "Storage.hpp"

void testLocalStorage();

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleStorage, nullptr };
	tp::ModuleManifest testModule("StorageTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testLocalStorage();

	testModule.deinitialize();
}