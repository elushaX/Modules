#pragma once

#include "Strings.hpp"
#include "List.hpp"
#include "Map.hpp"

namespace tp {

	struct CfGrammar {
		struct Rule {
			struct Arg {
				String id;
				bool isTerminal = false;
				bool isEpsilon = false;
			};

			String id;
			List<Arg> args;
		};

		List<Rule> rules;
		String startTerminal;

	private:
		struct NonTerminal {
			List<Rule*> rules;
			Map<String, NonTerminal*> references;
			Map<String, NonTerminal*> referencing;
		};

		struct Terminal {};

		Map<String, Terminal> mTerminals;
		Map<String, NonTerminal> mNonTerminals;

	public:
		static struct CfGrammarParserState* initializeCfGrammarParser();
		static void deinitializeCfGrammarParser(CfGrammarParserState*);

	public:
		bool parse(CfGrammarParserState* context, const String& source);
		bool compile();

		bool isAmbiguous();
		void optimize();
	};
}