
#include "AutomatasCommon.hpp"
#include "Utils.hpp"

using namespace tp;

static ModuleManifest* sModuleDependencies[] = { &gModuleUtils, nullptr };
ModuleManifest tp::gModuleAutomatas = ModuleManifest("Automatas", nullptr, nullptr, sModuleDependencies);
