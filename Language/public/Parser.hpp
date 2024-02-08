
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
		typedef RegularAutomata<tAlphabetType, TokenType> RegularAutomata;

		// ContextFreeGrammar;
		// ContextFreeCompiler;
		typedef FiniteStateAutomation<ContextFreeCompiler::SymbolID, ContextFreeCompiler::Item> ContextFreeGraph;
		typedef ContextFreeAutomata<ContextFreeCompiler::SymbolID, ContextFreeCompiler::Item> ContextFreeAutomata;

	public:
		Parser() = default;

	public:
		bool compileTables(const ContextFreeGrammar& cfGrammar, const RegularGrammar& reGrammar) {
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

			return true;
		}

		void parse(const tAlphabetType* sentence, ualni sentenceLength, AST& out) {}

	public:
		// save load compiled tables
		RegularAutomata mRegularAutomata;
		ContextFreeAutomata mContextFreeAutomata;
	};
}
