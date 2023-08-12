
#include "NewPlacement.hpp"
#include "Testing.hpp"
#include "Parser.hpp"

using namespace tp;

void testGrammar();
void testCLR();

int main(int argc, const char* argv[]) {
	tp::ModuleManifest* deps[] = { &tp::gModuleParser, nullptr };
	tp::ModuleManifest testModule("CommandLineTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testGrammar();
	testCLR();

	testModule.deinitialize();
	return 0;
}