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

			[[nodiscard]] bool isProductive() const {
				for (auto arg : args) {
					if (arg->id == id) return false;
				}
				return true;
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

			[[nodiscard]] bool isProductive() const {
				for (auto rule : rules) {
					if (rule->isProductive()) return true;
				}
				return false;
			}

			[[nodiscard]] bool isLooped(Map<String, ualni>& processed, const String& id) const {
				for (auto ref : referencing) {
					if (processed.presents(ref->key)) return true;
				}
				processed.put(id, {});
				for (auto ref : referencing) {
					if (ref->val->isLooped(processed, ref->key)) return true;
				}
				return false;
			}
		};

		struct Terminal {};

		Map<String, Terminal> mTerminals;
		Map<String, NonTerminal> mNonTerminals;
		bool mIsLooped = false;

	public:
		static struct CfGrammarParserState* initializeCfGrammarParser();
		static void deinitializeCfGrammarParser(CfGrammarParserState*);

	public:
		bool parse(CfGrammarParserState* context, const String& source);
		bool compile();

		void generateSentences(List<Sentence>& out);
		static void printSentence(Sentence& in);

		[[nodiscard]] bool isLooped() const;
	};
}