
#include "Tokenizer.hpp"
#include "Testing.hpp"

void testTokenizer();

int main(int argc, char* argv[]) {

	tp::ModuleManifest* ModuleDependencies[] = { &tp::gModuleTokenizer, nullptr };
	tp::ModuleManifest TestModule("TokenizerTest", nullptr, nullptr, ModuleDependencies);
	if (!TestModule.initialize()) {
		return 1;
	}

	testTokenizer();

	TestModule.deinitialize();
}