
#pragma once

#include "AST.hpp"
#include "RegularCompiler.hpp"

namespace tp {

	template <typename tAlphabetType, typename TokenType>
	class Parser {

		typedef TransitionMatrix<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularTable;
		typedef DFA<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularGraph;
		typedef RegularCompiler<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularCompiler;
		typedef NFA<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularNonDetGraph;

	public:
		Parser() = default;

	public:
		void compileTables(const ContextFreeGrammar& cfGrammar, const RegularGrammar<tAlphabetType, TokenType>& reGrammar) {
			// Compile Regular Grammar
			{
				RegularNonDetGraph nfa;
				RegularCompiler compiler;
				compiler.compile(nfa, reGrammar);

				RegularGraph dfa(nfa);
				mRegularTable.construct(dfa);
			}
		}

		void parse(const tAlphabetType* sentence, ualni sentenceLength, AST& out) {}

	public:
		// save load compiled tables
		RegularTable mRegularTable;
	};
}
