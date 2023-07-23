#include "GraphicsCommon.hpp"

namespace tp {
	static ModuleManifest* deps[] = { &gModuleStrings, &gModuleMath, nullptr };
	ModuleManifest gModuleGraphics = ModuleManifest("Graphics", nullptr, nullptr, deps);
}