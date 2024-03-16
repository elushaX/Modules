
#include "Test.hpp"

/*
using namespace tp;

void testAutomation() {

	FiniteStateAutomation<char, int> automata;

	auto start = automata.addState(0, false);
	auto end = automata.addState(1, true);

	automata.addTransition(start, end, 'a');

	automata.setStartState(start);

	automata.makeDeterministic();
}

void test() {
	auto parser = SimpleParser<int1>();

	parser.compileTables(gGrammar, String::Logic::calcLength(gGrammar));
	auto result = parser.parse(gSentence, String::Logic::calcLength(gSentence));
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleLanguage, nullptr };
	tp::ModuleManifest testModule("Test", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testAutomation();
	test();

	testModule.deinitialize();
}

*/

int main() { return 0; }