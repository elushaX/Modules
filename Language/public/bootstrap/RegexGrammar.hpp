
#pragma once

#include "Parser.hpp"

namespace tp {

	template <typename tAlphabetType>
	struct RegexDefinition {

		enum RTokens : alni {
			RInTransition = -1,
			RNone = 0,
			RChar,     // a literal char
			REscape,   // \X
			RString,   // "..."
			RPipe, RStar, RPlus, RQuestion, RLParen, RRParen,
			RLBracket, RRBracket, RMinus, RCaret
		};

		using RRegexGrammar = RegularGrammar<tAlphabetType, RTokens>;
		using RRegexNode = typename RRegexGrammar::Node;
		using RegexParser = Parser<tAlphabetType, RTokens, RInTransition, 0, 127>;


		// regex tokenizer character classes
		static const RRegexNode* rNot(RRegexGrammar& rg, tAlphabetType q);
		static const RRegexNode* rAnyChar(RRegexGrammar& rg);

		// regex tokenizer + structure
		static void buildRegexTokenizer(RRegexGrammar& rg);
		static void buildRegexStructure(ContextFreeGrammar& cfg, Map<String, RTokens>& tm);
	};
}

#include "bootstrap/RegexGrammar.ipp"
