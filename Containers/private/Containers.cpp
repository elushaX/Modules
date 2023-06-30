
#include "ContainersCommon.hpp"

#include <cstdlib>

namespace tp {

	static ModuleManifest* sModuleDependencies[] = { &gModuleBase,nullptr };
	ModuleManifest gModuleContainers = ModuleManifest("Containers", nullptr, nullptr, sModuleDependencies);

	void* DefaultAllocator::allocate(ualni size) {
		return malloc(size);
	}

	void DefaultAllocator::deallocate(void* p) {
		free(p);
	}
}