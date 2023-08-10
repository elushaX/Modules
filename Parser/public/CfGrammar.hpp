#pragma once

#include "Strings.hpp"
#include "List.hpp"

namespace tp {

	struct CfGrammar {
		struct Rule {
			struct Arg {
				String id;
				bool isTerminal;
			};

			String id;
			List<Arg> args;
		};

		List<Rule> rules;
		String startTerminal;

	public:
		static struct CfGrammarParserState* initializeCfGrammarParser();
		static void deinitializeCfGrammarParser(CfGrammarParserState*);

	public:
		void parse(CfGrammarParserState* context, const int1* source);
	};
}