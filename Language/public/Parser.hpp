
#pragma once

#include "compilers/RegularCompiler.hpp"
#include "automatas/DetFinateStateAutomata.hpp"

#include "compilers/ContextFreeCompiler.hpp"
#include "automatas/DetPushDownAutomata.hpp"

namespace tp {

	template <typename tAlphabetType, typename tTokenType, tTokenType tInTransition, ualni MinSymbol, ualni MaxSymbol>
	class Parser {

		using ParserRegularGrammar = RegularGrammar<tAlphabetType, tTokenType>;
		using ParserRegularCompiler = RegularCompiler<tAlphabetType, tTokenType, tInTransition, MinSymbol, MaxSymbol>;
		using RegularGraph = FiniteStateAutomation<tAlphabetType, tTokenType>;
		using ParserRegularAutomata = DetFinateStateAutomata<tAlphabetType, tTokenType>;

		using ContextFreeGraph = FiniteStateAutomation<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>;
		using ParserContextFreeAutomata = DetPushDownAutomata<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>;

	public:
		struct ParseResult {
			bool accepted = false;
			const ParserContextFreeAutomata::StackItem* ast = nullptr;
		};

	public:
		Parser() = default;

		void setIgnoreToken(tTokenType token);

	public:
		bool compileTables(
			const ContextFreeGrammar& cfGrammar,
			const ParserRegularGrammar& reGrammar,
			const Map<String, tTokenType>& contextFreeToRegular
		);

		ParseResult parse(const tAlphabetType* sentence, ualni sentenceLength);

	public:
		// save load compiled tables
		ParserRegularAutomata mRegularAutomata;
		ParserContextFreeAutomata mContextFreeAutomata;

		Map<tTokenType, ContextFreeCompiler::SymbolVal> mGrammarGlue;
		Map<ContextFreeCompiler::SymbolVal, String> mAstNames;

		tTokenType mIgnoreToken = tTokenType();
	};
}

#include "Parser.ipp"
