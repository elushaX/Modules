
#include "WidgetBase.hpp"

namespace tp {
	GlobalGUIConfig* gGlobalGUIConfig = nullptr;

	ModuleManifest* deps[] = { &gModuleStrings, nullptr };
	ModuleManifest gModuleWidgets = ModuleManifest("Widgets", nullptr, nullptr, deps);
}