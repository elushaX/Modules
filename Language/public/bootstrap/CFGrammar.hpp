
#pragma once

#include "Parser.hpp"

namespace tp {

	template <typename tAlphabetType>
	struct CFGDefinition {

		enum GTokens : alni {
			GInTransition = -1,
			GIdent,
			GLiteral,  // '...'
			GRegex,    // "..."
			GAction,   // [...]
			GPercent, GLBrace, GRBrace, GColon, GSemi, GPipe,
			GIgnore
		};

		using GRegexGrammar = RegularGrammar<tAlphabetType, GTokens>;
		using GRegexNode = typename GRegexGrammar::Node;
		using GrammarParser = Parser<tAlphabetType, GTokens, GInTransition, 0, 127>;

		// .g tokenizer character classes
		static const GRegexNode* gLetter(GRegexGrammar& rg);
		static const GRegexNode* gAlnum(GRegexGrammar& rg);
		static const GRegexNode* gNot(GRegexGrammar& rg, tAlphabetType q);

		// .g tokenizer + structure
		static void buildGrammarTokenizer(GRegexGrammar& rg);
		static void buildGrammarStructure(ContextFreeGrammar& cfg, Map<String, GTokens>& tm);
	};
}

#include "bootstrap/CFGrammar.ipp"
