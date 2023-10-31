
#include "NewPlacement.hpp"

#include "SimpleParser.hpp"

using namespace tp;

SimpleParser::SimpleParser() {
	// Grammar for unified grammar format sentence that tables compiled from

	// Define Context-Free grammar
	ContextFreeGrammar contextFreeGrammar;
	{
		// use existing CF grammar interface
		auto term = contextFreeGrammar.createTerminal();

		auto nonTerm = contextFreeGrammar.createNonTerminal();
		auto nonTerm2 = contextFreeGrammar.createNonTerminal();

		contextFreeGrammar.addRule(nonTerm, { term, nonTerm2 });
		contextFreeGrammar.addRule();

		contextFreeGrammar.setStart(nonTerm);
	}

	// Define Regular grammar
	RegularGrammar<uint1, int1> rg;
	{
		// this is basically ast from existing tokenizer
		rg.addRule(rg.seq({ rg.val('a'), rg.val('b') }), 0);
	}

	mUnifiedGrammarParser.compileTables(contextFreeGrammar, rg);
}

void SimpleParser::compileTables(const Sentence& grammar) {
	AST unifiedGrammarAst;
	mUnifiedGrammarParser.parse(grammar, unifiedGrammarAst);

	// compile each ast into RegularGrammar and ContextFree Grammar api instructions
	ContextFreeGrammar userContextFreeGrammar;
	RegularGrammar<uint1, int1> userRegularGrammar;

	// ...
	// split ast into RE and CF part
	// generate and execute grammar api commands
	// use existing tokenizer code to create RE transition matrix
	// ...
	// compile tables from user grammar

	mUserParser.compileTables(userContextFreeGrammar, userRegularGrammar);
}

void SimpleParser::parse(const Sentence& sentence, AST& out) { mUserParser.parse(sentence, out); }