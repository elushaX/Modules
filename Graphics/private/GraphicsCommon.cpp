#include "GraphicsCommon.hpp"
#include "MathCommon.hpp"

namespace tp {
	static ModuleManifest* deps[] = { &gModuleAllocators, nullptr };
	ModuleManifest gModuleGraphics = ModuleManifest("Graphics", nullptr, nullptr, deps);
}