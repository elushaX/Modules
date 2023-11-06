
#pragma once

#include "AST.hpp"
#include "GrammarCompiler.hpp"

namespace tp {

	template <typename tAlphabetType>
	class Parser {
	public:
		Parser() = default;

	public:
		void compileTables(const ContextFreeGrammar& cfGrammar, const RegularGrammar<tAlphabetType, ualni>& reGrammar) {}
		void parse(const tAlphabetType* sentence, ualni sentenceLength, AST& out) {}

	public:
		// save load compiled tables
	};
}
