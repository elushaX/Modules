
#include "Strings.hpp"
#include "Allocators.hpp"
#include "Logging.hpp"

using namespace tp;

bool initialize(const ModuleManifest*) {
	Logger::initializeGlobal();
	return true;
}

void deinitialize(const ModuleManifest*) {
	Logger::deinitializeGlobal();
}

static tp::ModuleManifest* sModuleDependencies[] = {
	&tp::gModuleContainers,
	&tp::gModuleAllocators,
	nullptr
};

ModuleManifest tp::gModuleStrings = ModuleManifest("Strings", initialize, deinitialize, sModuleDependencies);