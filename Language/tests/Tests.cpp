
#include "NewPlacement.hpp"
#include "Test.hpp"

using namespace tp;

void test() {
	auto parser = SimpleParser<int1>();
	auto ast = AST();

	parser.compileTables(gGrammar, String::Logic::calcLength(gGrammar));
	parser.parse(gSentence, String::Logic::calcLength(gSentence), ast);
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleLanguage, nullptr };
	tp::ModuleManifest testModule("Test", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	test();

	testModule.deinitialize();
}
