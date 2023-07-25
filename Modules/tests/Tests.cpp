
#include "Module.hpp"

void testArchiver();

int main() {
	tp::ModuleManifest* ModuleDependencies[] = { &tp::gModuleBase, nullptr };
	tp::ModuleManifest TestModule("Test", nullptr, nullptr, ModuleDependencies);

	if (!TestModule.initialize()) {
		return 1;
	}

	ASSERT(tp::gEnvironment.mWidth == tp::Environment::ArchWidth::X64);

	ASSERT(tp::max(2, 1) == 2)
	ASSERT(tp::max(-1, 0) == 0)
	ASSERT(tp::max(0, -1) == 0)
	ASSERT(tp::min(0, -1) == -1)

	testArchiver();

	TestModule.deinitialize();
}