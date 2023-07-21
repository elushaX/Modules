
#include "ConnectionCommon.hpp"

static tp::ModuleManifest* sModuleDependencies[] = {
		&tp::gModuleStrings,
		nullptr
};

tp::ModuleManifest tp::gModuleConnection = ModuleManifest("Storage", nullptr, nullptr, sModuleDependencies);