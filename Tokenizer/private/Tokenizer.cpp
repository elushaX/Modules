
#include "Tokenizer.hpp"

using namespace tp;

static ModuleManifest* sModuleDependencies[] = {
	&tp::gModuleMath, 
	&tp::gModuleStrings,
	nullptr
};

void deinit(const ModuleManifest*) {}

ModuleManifest tp::gModuleTokenizer = ModuleManifest("Tokenizer", nullptr, deinit, sModuleDependencies);

