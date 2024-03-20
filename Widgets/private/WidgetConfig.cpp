
#include "WidgetBase.hpp"

#include "Graphics.hpp"

namespace tp {
	GlobalGUIConfig* gGlobalGUIConfig = nullptr;

	ModuleManifest* deps[] = { &gModuleGraphics, &gModuleStrings, nullptr };
	ModuleManifest gModuleWidgets = ModuleManifest("Widgets", nullptr, nullptr, deps);
}