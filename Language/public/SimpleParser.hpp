
#pragma once

#include "Parser.hpp"

#include "bootstrap/CFGrammar.hpp"
#include "bootstrap/RegexGrammar.hpp"

namespace tp {

	// Gives ability to express grammar in the Unified Format as sentence
	template <typename tAlphabetType>
	class SimpleParser {

		using UGParser = typename CFGDefinition<tAlphabetType>::GrammarParser;
		using UGRegexTokensParser = typename RegexDefinition<tAlphabetType>::RegexParser;

		using UserParser = Parser<tAlphabetType, alni, -1, 0, 127>;

	public:
		SimpleParser();

	public:
		bool compileTables(const tAlphabetType* grammar, ualni grammarLength);

		typename UserParser::ParseResult parse(const tAlphabetType* grammar, ualni grammarLength);

		[[nodiscard]] bool isCompiled() const;

		// ---- semantic-action accessors for walking the result AST ----
		using StackItem = DetPushDownAutomata<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>::StackItem;

		// action tag assigned to the production that reduced this node (empty if none / terminal)
		[[nodiscard]] String actionOf(const StackItem* node) const;

		// source text covered by a node's terminal symbol
		[[nodiscard]] String lexemeOf(const tAlphabetType* source, const StackItem* node) const;

	private:
		UGParser mUnifiedGrammarParser;
		UGRegexTokensParser mRegexParser;

		Map<String, typename CFGDefinition<tAlphabetType>::GTokens> mUnifiedTerminals;
		Map<String, typename RegexDefinition<tAlphabetType>::RTokens> mRegexTerminals;

		Buffer<String> mActions; // action tag per user production (compiler rule index - 1)

		bool mCompileOk = false;

		UserParser mUserParser;
	};
}

#include "SimpleParser.ipp"
