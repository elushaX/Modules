
#include "NewPlacement.hpp"
#include "Testing.hpp"
#include "Parser.hpp"

using namespace tp;

TEST_DEF(Grammar) {

	String source = R"(
Start Body
Body : [ Function ] | List | &;
List : [ LIST ];
)";

	auto state = CfGrammar::initializeCfGrammarParser();

	CfGrammar grammar;

	if (!grammar.parse(state, source)) {
		TEST(0 && "Parsing is failed\n");
		CfGrammar::deinitializeCfGrammarParser(state);
		return;
	}

	if (!grammar.compile()) {
		TEST(0 && "Compilation is failed\n");
		CfGrammar::deinitializeCfGrammarParser(state);
		return;
	}

	CfGrammar::deinitializeCfGrammarParser(state);

	printf("Grammar accepted.\n");

	List<CfGrammar::Sentence> sentences;
	grammar.generateSentences(sentences);

	TEST_ASSERT(sentences.length() == 3);
	TEST_ASSERT(sentences.first()->data.terms.length() == 1);
	TEST_ASSERT(sentences.last()->data.terms.length() == 1);

	TEST(sentences.first()->data.terms.first()->data.id ==  "Function");
	TEST(sentences.last()->data.terms.first()->data.id == "LIST");

	printf("Example sentences formed from grammar: \n");
	for (auto sentence : sentences) tp::CfGrammar::printSentence(sentence.data());
}