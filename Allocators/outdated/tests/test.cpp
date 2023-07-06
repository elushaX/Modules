
#include "allocators.hpp"

int main() {

	tp::ModuleManifest* ModuleDependencies[] = { &tp::gModuleAllocator, NULL };
	tp::ModuleManifest TestModule("Test", NULL, NULL, ModuleDependencies);
	if (!TestModule.initialize()) {
		return 1;
	}

	tp::HeapAllocGlobal alloc;

	int* val = new(alloc) int();
	delete(alloc, val);

	TestModule.deinitialize();
}