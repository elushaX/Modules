
#pragma once

#include "Parser.hpp"

namespace tp {

	// =====================================================================
	// Defines the Unified Grammar format (the .g syntax + the inline regex
	// syntax) via the existing C++ grammar interface. Only builds grammars.
	// =====================================================================
	template <typename tAlphabetType>
	struct UnifiedGrammarDefinition {

		// .g grammar tokens
		enum GTokens : alni {
			GInTransition = -1,
			GIdent,
			GLiteral,  // '...'
			GRegex,    // "..."
			GAction,   // [...]
			GPercent, GLBrace, GRBrace, GColon, GSemi, GPipe,
			GIgnore
		};

		// regex tokens (0 is reserved so it never collides with the ignore token)
		enum RTokens : alni {
			RInTransition = -1,
			RNone = 0,
			RChar,     // a literal char
			REscape,   // \X
			RString,   // "..."
			RPipe, RStar, RPlus, RQuestion, RLParen, RRParen,
			RLBracket, RRBracket, RMinus, RCaret
		};

		using GRegexGrammar = RegularGrammar<tAlphabetType, GTokens>;
		using GRegexNode = typename GRegexGrammar::Node;
		using GrammarParser = Parser<tAlphabetType, GTokens, GInTransition, 0, 127>;

		using RRegexGrammar = RegularGrammar<tAlphabetType, RTokens>;
		using RRegexNode = typename RRegexGrammar::Node;
		using RegexParser = Parser<tAlphabetType, RTokens, RInTransition, 0, 127>;

		// .g tokenizer character classes
		static const GRegexNode* gLetter(GRegexGrammar& rg);
		static const GRegexNode* gAlnum(GRegexGrammar& rg);
		static const GRegexNode* gNot(GRegexGrammar& rg, tAlphabetType q);

		// regex tokenizer character classes
		static const RRegexNode* rNot(RRegexGrammar& rg, tAlphabetType q);
		static const RRegexNode* rAnyChar(RRegexGrammar& rg);

		// .g tokenizer + structure
		static void buildGrammarTokenizer(GRegexGrammar& rg);
		static void buildGrammarStructure(ContextFreeGrammar& cfg, Map<String, GTokens>& tm);

		// regex tokenizer + structure
		static void buildRegexTokenizer(RRegexGrammar& rg);
		static void buildRegexStructure(ContextFreeGrammar& cfg, Map<String, RTokens>& tm);
	};

	// =====================================================================
	// Walks a parsed Unified Grammar AST and generates the target grammar:
	// a ContextFreeGrammar (productions + precedence) and a RegularGrammar
	// (tokenizer), executed through the existing grammar api.
	// =====================================================================
	template <typename tAlphabetType>
	struct UnifiedGrammarTranslator {

		using Def = UnifiedGrammarDefinition<tAlphabetType>;
		using GrammarParser = typename Def::GrammarParser;
		using RegexParser = typename Def::RegexParser;

		using Automata = ContextFreeAutomata<ContextFreeCompiler::SymbolVal, ContextFreeCompiler::Item>;
		using AstNode = const typename Automata::StackItem*;

		using UserRegexGrammar = RegularGrammar<tAlphabetType, alni>;
		using UserRegexNode = typename UserRegexGrammar::Node;

		struct BuildCtx {
			const tAlphabetType* text = nullptr;
			ContextFreeGrammar* cf = nullptr;
			UserRegexGrammar* rg = nullptr;
			Map<String, alni>* terminals = nullptr;
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
		void translate(
			AstNode root,
			const tAlphabetType* text,
			ContextFreeGrammar& cf,
			UserRegexGrammar& rg,
			Map<String, alni>& terminals,
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
