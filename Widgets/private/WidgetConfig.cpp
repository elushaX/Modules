
#include "WidgetBase.hpp"

namespace tp {
	GlobalGUIConfig* gGlobalGUIConfig = nullptr;

	ModuleManifest* deps[] = { &gModuleMath, &gModuleStrings, nullptr };
	ModuleManifest gModuleWidgets = ModuleManifest("Widgets", nullptr, nullptr, deps);
}