
#pragma once

#include "RegularCompiler.hpp"
#include "RegularAutomata.hpp"

#include "ContextFreeCompiler.hpp"
#include "ContextFreeAutomata.hpp"

namespace tp {

	template <typename tAlphabetType, typename tTokenType, tTokenType tInTransition, ualni MinSymbol, ualni MaxSymbol>
	class Parser {

		using ParserRegularGrammar = RegularGrammar<tAlphabetType, tTokenType>;
		using ParserRegularCompiler = RegularCompiler<tAlphabetType, tTokenType, tInTransition, MinSymbol, MaxSymbol>;
		using RegularGraph = FiniteStateAutomation<tAlphabetType, tTokenType>;
		using ParserRegularAutomata = RegularAutomata<tAlphabetType, tTokenType>;

		// ContextFreeGrammar;
		// ContextFreeCompiler;
		using ContextFreeGraph = FiniteStateAutomation<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>;
		using ParserContextFreeAutomata = ContextFreeAutomata<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>;

	public:
		struct ParseResult {
			bool accepted = false;
			const ParserContextFreeAutomata::StackItem* ast = nullptr;
		};

	public:
		Parser() = default;

	public:
		bool compileTables(
			const ContextFreeGrammar& cfGrammar,
			const ParserRegularGrammar& reGrammar,
			const Map<String, tTokenType>& contextFreeToRegular
		) {
			// Compile Regular Grammar
			{
				RegularGraph graph;
				ParserRegularCompiler compiler;
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
			Buffer<ContextFreeCompiler::SymbolVal> tokens;

			const tAlphabetType* sentenceIter = sentence;
			ualni lengthIter = sentenceLength;

			while (lengthIter) {
				auto result = mRegularAutomata.accept(sentenceIter, lengthIter);

				if (!result.accepted) {
					return { false, nullptr };
				}

				tokens.append(ContextFreeCompiler::SymbolVal(
					mGrammarGlue.get(result.state), ualni(sentenceIter - sentence), result.advancedIdx
				));

				sentenceIter += result.advancedIdx;
				lengthIter -= result.advancedIdx;
			}

			ParserContextFreeAutomata::AcceptResult result = mContextFreeAutomata.accept(tokens.getBuff(), tokens.size());
			return { result.accepted, result.ast };
		}

	public:
		// save load compiled tables
		ParserRegularAutomata mRegularAutomata;
		ParserContextFreeAutomata mContextFreeAutomata;

		Map<tTokenType, ContextFreeCompiler::SymbolVal> mGrammarGlue;
		Map<ContextFreeCompiler::SymbolVal, String> mAstNames;
	};
}
