#pragma once

#include "Parser.hpp"

namespace tp {

	// Gives ability to express grammar in the Unified Format as sentence
	template <typename tAlphabetType>
	class SimpleParser {
	public:
		SimpleParser() {
			// Grammar for unified grammar format sentence that tables compiled from

			// Define Context-Free grammar
			ContextFreeGrammar contextFreeGrammar;
			{
				// use existing CF grammar interface
				contextFreeGrammar.addRule("a", { ContextFreeGrammar::Arg("") });
				contextFreeGrammar.setStart("a");
			}

			// Define Regular grammar
			RegularGrammar<tAlphabetType, ualni> rg;
			{
				// this is basically ast from existing tokenizer
				rg.addRule(rg.seq({ rg.val('a'), rg.val('b') }), 0);
			}

			mUnifiedGrammarParser.compileTables(contextFreeGrammar, rg);
		}

	public:
		void compileTables(const tAlphabetType* grammar, ualni grammarLength) {
			AST unifiedGrammarAst;
			mUnifiedGrammarParser.parse(grammar, grammarLength, unifiedGrammarAst);

			// compile each ast into RegularGrammar and ContextFree Grammar api instructions
			ContextFreeGrammar userContextFreeGrammar;
			RegularGrammar<tAlphabetType, ualni> userRegularGrammar;

			// ...
			// split ast into RE and CF part
			// generate and execute grammar api commands
			// use existing tokenizer code to create RE transition matrix
			// ...
			// compile tables from user grammar

			mUserParser.compileTables(userContextFreeGrammar, userRegularGrammar);
		}

		void parse(const tAlphabetType* grammar, ualni grammarLength, AST& out) { mUserParser.parse(grammar, grammarLength, out); }

	private:
		Parser<tAlphabetType> mUnifiedGrammarParser;
		Parser<tAlphabetType> mUserParser;
	};
}
