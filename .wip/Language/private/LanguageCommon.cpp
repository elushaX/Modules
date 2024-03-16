
#include "LanguageCommon.hpp"
#include "Strings.hpp"

using namespace tp;

static ModuleManifest* sModuleDependencies[] = { &gModuleStrings, nullptr };
ModuleManifest tp::gModuleLanguage = ModuleManifest("Language", nullptr, nullptr, sModuleDependencies);
