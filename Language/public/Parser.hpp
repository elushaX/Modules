
#pragma once

#include "RegularCompiler.hpp"
#include "RegularAutomata.hpp"

#include "ContextFreeCompiler.hpp"
#include "ContextFreeAutomata.hpp"

namespace tp {

	template <typename tAlphabetType, typename tTokenType, tTokenType tInTransition, ualni MinSymbol, ualni MaxSymbol>
	class Parser {

		typedef RegularGrammar<tAlphabetType, tTokenType> RegularGrammar;
		typedef RegularCompiler<tAlphabetType, tTokenType, tInTransition, MinSymbol, MaxSymbol> RegularCompiler;
		typedef FiniteStateAutomation<tAlphabetType, tTokenType> RegularGraph;
		typedef RegularAutomata<tAlphabetType, tTokenType> RegularAutomata;

		// ContextFreeGrammar;
		// ContextFreeCompiler;
		typedef FiniteStateAutomation<ContextFreeCompiler::SymbolId, ContextFreeCompiler::Item> ContextFreeGraph;
		typedef ContextFreeAutomata<ContextFreeCompiler::SymbolId, ContextFreeCompiler::Item> ContextFreeAutomata;

	public:
		struct ParseResult {
			bool accepted = false;
			const ContextFreeAutomata::StackItem* ast = nullptr;
		};

	public:
		Parser() = default;

	public:
		bool compileTables(
			const ContextFreeGrammar& cfGrammar,
			const RegularGrammar& reGrammar,
			const Map<String, tTokenType>& contextFreeToRegular
		) {
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

				// make glue
				for (auto symbol : *compiler.getSymbols()) {
					auto symbolId = compiler.getSymbolId(symbol->mId);
					if (symbol->mIsTerminal) {
						auto iter = contextFreeToRegular.presents(symbol->mId);
						if (!iter) return false;
						mGrammarGlue.put(contextFreeToRegular.getSlotVal(iter), symbolId);
					} else {
						mAstNames.put(symbolId, symbol->mId);
					}
				}
			}

			return true;
		}

		ParseResult parse(const tAlphabetType* sentence, ualni sentenceLength) {
			// get tokens stream
			Buffer<ContextFreeCompiler::SymbolId> tokens;

			const tAlphabetType* sentenceIter = sentence;
			ualni lengthIter = sentenceLength;

			while (lengthIter) {
				auto result = mRegularAutomata.accept(sentenceIter, lengthIter);

				if (!result.accepted) {
					return { false, nullptr };
				}

				sentenceIter += result.advancedIdx;
				lengthIter -= result.advancedIdx;

				tokens.append(mGrammarGlue.get(result.state));
			}

			ContextFreeAutomata::AcceptResult result = mContextFreeAutomata.accept(tokens.getBuff(), tokens.size());
			return { result.accepted, result.ast };
		}

	public:
		// save load compiled tables
		RegularAutomata mRegularAutomata;
		ContextFreeAutomata mContextFreeAutomata;

		Map<tTokenType, ContextFreeCompiler::SymbolId> mGrammarGlue;
		Map<ContextFreeCompiler::SymbolId, String> mAstNames;
	};
}
