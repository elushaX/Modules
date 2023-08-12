#include "NewPlacement.hpp"
#include "Parser.hpp"

#include "Tokenizer.hpp"

using namespace tp;

static ModuleManifest* sModuleDependencies[] = { &gModuleTokenizer, nullptr };
ModuleManifest tp::gModuleParser = ModuleManifest("CommandLine", nullptr, nullptr, sModuleDependencies);
