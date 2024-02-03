
#include "Test.hpp"

using namespace tp;

void test() {
	auto parser = SimpleParser();
	auto grammar = Sentence(gGrammar);

	auto sentence = Sentence(gSentence);
	auto ast = AST();

	parser.compileTables(grammar);
	parser.parse(sentence, ast);
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
