
#pragma once

#include "Parser.hpp"

#include "bootstrap/CFGrammar.hpp"
#include "bootstrap/RegexGrammar.hpp"

namespace tp {

	template <typename tAlphabetType>
	struct UnifiedGrammarTranslator {

		using GrammarParser = typename CFGDefinition<tAlphabetType>::GrammarParser;
		using RegexParser = typename RegexDefinition<tAlphabetType>::RegexParser;

		using Automata = DetPushDownAutomata<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>;
		using AstNode = const typename Automata::StackItem*;

		using UserRegexGrammar = RegularGrammar<tAlphabetType, alni>;
		using UserRegexNode = typename UserRegexGrammar::Node;

		struct BuildCtx {
			const tAlphabetType* text = nullptr;
			ContextFreeGrammar* cf = nullptr;
			UserRegexGrammar* rg = nullptr;
			Map<String, alni>* terminals = nullptr;
			Buffer<String>* actions = nullptr; // action tag per CFG production (parallel to cf rules)
			String start;
			bool hasStart = false;
			alni nextToken = 0;
			ualni nextPrecedence = 1;
			Buffer<AstNode> productionNodes;
			Map<String, bool> namedTerminals;
			String whitespace;
			bool hasWhitespace = false;
		};

		// token 0 of the user tokenizer is reserved for whitespace (the ignore token)
		// actions receives one entry per CFG production (same order as cf.getRules())
		void translate(
			AstNode root,
			const tAlphabetType* text,
			ContextFreeGrammar& cf,
			UserRegexGrammar& rg,
			Map<String, alni>& terminals,
			Buffer<String>& actions,
			GrammarParser& grammarParser,
			RegexParser& regexParser
		);

	private:
		static bool strEq(const char* a, const char* b);
		static String lexeme(const tAlphabetType* text, const ContextFreeCompiler::SymbolVal& sym);
		static char decodeEscape(char e);
		static String stripQuotes(const String& text);

		String nodeName(AstNode node);
		String regexNodeName(AstNode node);

		// grammar AST walk
		void walkGrammar(AstNode node, BuildCtx& ctx);
		void walkBody(AstNode node, BuildCtx& ctx);
		void walkItem(AstNode node, BuildCtx& ctx);
		void walkDirective(AstNode node, BuildCtx& ctx);
		void collectArgTexts(AstNode args, BuildCtx& ctx, Buffer<String>& out);
		void collectProduction(AstNode node, BuildCtx& ctx);
		bool isRegexSymbol(AstNode sym, BuildCtx& ctx);
		void buildProduction(AstNode node, BuildCtx& ctx);
		String altAction(AstNode altNode, BuildCtx& ctx);
		void collectAlts(AstNode alts, Buffer<AstNode>& out);
		void collectAltsRest(AstNode rest, Buffer<AstNode>& out);
		void collectSymbolNodes(AstNode symbols, Buffer<AstNode>& out);

		// regex string -> RegularGrammar (via mRegexParser)
		const UserRegexNode* compileRegexString(const String& regex, BuildCtx& ctx);
		String rLexeme(const ContextFreeCompiler::SymbolVal& sym);
		const UserRegexNode* compileAlt(AstNode alt, BuildCtx& ctx);
		const UserRegexNode* compileAltRest(AstNode rest, const UserRegexNode* left, BuildCtx& ctx);
		const UserRegexNode* compileConcat(AstNode concat, BuildCtx& ctx);
		const UserRegexNode* compileConcatRest(AstNode rest, const UserRegexNode* left, BuildCtx& ctx);
		const UserRegexNode* compileRepeat(AstNode repeat, BuildCtx& ctx);
		const UserRegexNode* compileAtom(AstNode atom, BuildCtx& ctx);
		const UserRegexNode* charSeq(const String& text, BuildCtx& ctx);
		const UserRegexNode* compileClass(AstNode cls, BuildCtx& ctx);
		char classCharValue(AstNode classChar, BuildCtx& ctx);
		void collectClassItems(AstNode items, Buffer<IterRange<tAlphabetType>>& out, BuildCtx& ctx);

	private:
		GrammarParser* mGrammarParser = nullptr;
		RegexParser* mRegexParser = nullptr;
		String mRegexText;
	};
}

#include "bootstrap/Translator.ipp"
