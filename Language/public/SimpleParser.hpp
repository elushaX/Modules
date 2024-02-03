#pragma once

#include "Parser.hpp"

namespace tp {

	// Gives ability to express grammar in the Unified Format as sentence
	class SimpleParser {
	public:
		SimpleParser();

	public:
		void compileTables(const Sentence& grammar);
		void parse(const Sentence& sentence, AST& out);

	private:
		Parser mUnifiedGrammarParser;
		Parser mUserParser;
	};
}
