#pragma once

#include "Strings.hpp"
#include "Map.hpp"
#include "List.hpp"

namespace tp {

	template <typename tAlphabetType>
	class ContextFreeGrammar {
	public:
		class NonTerminalSymbol;

		class RHSArg {
			tAlphabetType terminal;
			const NonTerminalSymbol* nonTerminal = nullptr;
		public:
			RHSArg(const NonTerminalSymbol* aNonTerminal) : nonTerminal(aNonTerminal) {}
			RHSArg(tAlphabetType terminal) : terminal(terminal) {}
		};

		class NonTerminalSymbol {
			String name;
			String description;

			struct Rule {
				List<RHSArg> mArgs;
			};

			List<Rule> mRules;

			Rule* newRule() {
				mRules.pushBack();
				return mRules.last()->data;
			}
		};

	private:
		Map<String, NonTerminalSymbol> mNonTerminals;
		NonTerminalSymbol* mStart;

	public:
		ContextFreeGrammar() = default;

		void setStartNonTerminal(const NonTerminalSymbol* start) {
			mStart = start;
		}

		const NonTerminalSymbol* addRule(const String& LHS, const InitialierList<RHSArg> RHS) {
			NonTerminalSymbol* nonTerminal = nullptr;
			auto idx = mNonTerminals.presents(LHS);
			if (idx) {
				nonTerminal = mNonTerminals.get(idx);
			} else {
				nonTerminal = new NonTerminalSymbol(LHS, "None");
				mNonTerminals.put(LHS, nonTerminal);
			}

			auto* rule = nonTerminal->newRule();
			for (auto arg : RHS) {
				rule->pushBack(arg);
			}

			return nonTerminal;
		}

		bool checkSemantic() {
			return false;
		}

		bool checkNonAmbiguous() {
			return false;
		}
	};

	template <typename tAlphabetType>
	class Parser {
		typedef ContextFreeGrammar<tAlphabetType> Grammar;

	public:
		class ASTNode {
			const Grammar::NonTerminalSymbol* mNonTerminal;
			List<const ASTNode*> mLeafs;
		};

	private:
		class InputTape {
		};

		class StateStack {
		};

	private:
		Grammar mGrammar;

	public:
		Parser() = default;

		Grammar& getGrammar() { return mGrammar; }

		void buildTables() {}

		const ASTNode* parse() {}
	};
}