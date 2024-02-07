
#pragma once

#include "AST.hpp"

#include "RegularCompiler.hpp"
#include "RegularAutomata.hpp"

#include "ContextFreeCompiler.hpp"
#include "ContextFreeAutomata.hpp"

namespace tp {

	template <typename tAlphabetType, typename TokenType, ualni MinSymbol, ualni MaxSymbol>
	class Parser {

		typedef RegularGrammar<tAlphabetType, TokenType> RegularGrammar;
		typedef RegularCompiler<tAlphabetType, TokenType, MinSymbol, MaxSymbol> RegularCompiler;
		typedef FiniteStateAutomation<tAlphabetType, TokenType> RegularGraph;
		typedef RegularAutomata<tAlphabetType, TokenType, TokenType::InTransition, TokenType::Failed> RegularAutomata;

		// ContextFreeGrammar;
		// ContextFreeCompiler;
		typedef FiniteStateAutomation<ContextFreeCompiler::Alphabet, ContextFreeCompiler::Item> ContextFreeGraph;
		typedef ContextFreeAutomata<ContextFreeCompiler::Alphabet, ContextFreeCompiler::Item> ContextFreeAutomata;

	public:
		Parser() = default;

	public:
		void compileTables(const ContextFreeGrammar& cfGrammar, const RegularGrammar& reGrammar) {
			// Compile Regular Grammar
			{
				RegularGraph graph;
				RegularCompiler compiler;
				compiler.compile(graph, reGrammar);
				graph.makeDeterministic();
				mRegularAutomata.construct(graph);
			}

			// compile context free grammar
			{
				ContextFreeGraph graph;
				ContextFreeCompiler compiler;
				compiler.compile(cfGrammar, graph);
				graph.makeDeterministic();
				mContextFreeAutomata.construct(graph);
			}
		}

		void parse(const tAlphabetType* sentence, ualni sentenceLength, AST& out) {}

	public:
		// save load compiled tables
		RegularAutomata mRegularAutomata;
		ContextFreeAutomata mContextFreeAutomata;
	};
}
