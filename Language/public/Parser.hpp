
#pragma once

#include "AST.hpp"
#include "GrammarCompiler.hpp"
#include "Sentence.hpp"

namespace tp {
	class Parser {
	public:
		Parser() = default;

	public:
		void compileTables(const ContextFreeGrammar& cfGrammar, const RegularGrammar& reGrammar);
		void parse(const Sentence& sentence, AST& out);

	public:
		// save load compiled tables
	};
}
