#include "DataAnalysisCommon.hpp"
#include "Allocators.hpp"
#include "ContainersCommon.hpp"
#include "MathCommon.hpp"

namespace tp {
	static ModuleManifest* deps[] = { &gModuleMath, &gModuleContainers, &gModuleAllocators, nullptr };
	ModuleManifest gModuleDataAnalysis = ModuleManifest("DataAnalysis", nullptr, nullptr, deps);
}
