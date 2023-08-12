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

				bool operator==(const Arg& in) const {
					return (id == in.id) && (isEpsilon == in.isEpsilon) && (isTerminal == in.isTerminal);
				}
			};

			String id;
			List<Arg> args;

			bool operator==(const Rule& in) const {
				return (id == in.id) && (args == in.args);
			}
		};

		struct Sentence {
			struct Term {
				String id;
				bool terminal;
				bool operator==(const Term& in) const {
					return (id == in.id) && (terminal == in.terminal);
				}
			};
			List<Term> terms;
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

		void generateSentences(List<Sentence>& out);
		static void printSentence(Sentence& in);

	private:
		void optimize();
	};
}