#pragma once

#include "Parser.hpp"

namespace tp {

	// Gives ability to express grammar in the Unified Format as sentence
	template <typename tAlphabetType>
	class SimpleParser {
		enum UGTokens : alni { InTransition = -1, TestSeq };

		typedef Parser<tAlphabetType, UGTokens, InTransition, 0, 127> UGParser;
		typedef Parser<tAlphabetType, alni, -1, 0, 127> UserParser;

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
			RegularGrammar<tAlphabetType, UGTokens> regularGrammar;
			{
				// this is basically ast from existing tokenizer
				regularGrammar.addRule(regularGrammar.seq({ regularGrammar.val('a'), regularGrammar.val('b') }), TestSeq);
			}

			Map<String, UGTokens> terminalsMap;
			terminalsMap.put("TestSeq", TestSeq);

			mUnifiedGrammarParser.compileTables(contextFreeGrammar, regularGrammar, terminalsMap);
		}

	public:
		void compileTables(const tAlphabetType* grammar, ualni grammarLength) {
			mUnifiedGrammarParser.parse(grammar, grammarLength);

			// compile each ast into RegularGrammar and ContextFree Grammar api instructions
			ContextFreeGrammar userContextFreeGrammar;
			RegularGrammar<tAlphabetType, alni> userRegularGrammar;

			// ...
			// split ast into RE and CF part
			// generate and execute grammar api commands
			// use existing tokenizer code to create RE transition matrix
			// ...
			// compile tables from user grammar

			Map<String, alni> terminalsMap;
			terminalsMap.put("TestSeq", 0);

			mUserParser.compileTables(userContextFreeGrammar, userRegularGrammar, terminalsMap);
		}

		UserParser::ParseResult parse(const tAlphabetType* grammar, ualni grammarLength) {
			return mUserParser.parse(grammar, grammarLength);
		}

	private:
		UGParser mUnifiedGrammarParser;
		UserParser mUserParser;
	};
}
