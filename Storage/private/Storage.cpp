
#include "Storage.hpp"

static tp::ModuleManifest* sModuleDependencies[] = {
		&tp::gModuleStrings,
		nullptr
};

tp::ModuleManifest tp::gModuleStorage = ModuleManifest("Storage", nullptr, nullptr, sModuleDependencies);