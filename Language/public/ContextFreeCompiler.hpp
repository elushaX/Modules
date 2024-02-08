
#pragma once

#include "Automata.hpp"
#include "Grammar.hpp"

namespace tp {

	class ContextFreeCompiler {

		typedef ualni SymbolID;

		struct Symbol {
			String mId;
			bool mIsTerminal = false;
		};

		struct Item {
			const ContextFreeGrammar::Rule* mRule = nullptr;
			ualni mAdvanceIdx = 0;
		};

		struct NonTerminal {
			Buffer<ContextFreeGrammar::Rule*> rules;
			Map<String, NonTerminal*> references;
			Map<String, NonTerminal*> referencing;

		public:
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

	public:
		bool compile(const ContextFreeGrammar& grammar, FiniteStateAutomation<SymbolID, Item>& automata) {
			if (!init(grammar)) return false;
			return true;
		}

	private:
		bool init(const ContextFreeGrammar& grammar) {

			if (!grammar.getRules()->size()) {
				return false;
			}

			for (auto rule : *grammar.getRules()) {
				if (!rule->getArgs()->size()) {
					return false;
				}
			}

			findNonTerminals(grammar);

			for (auto nonTerminal : mNonTerminals) {
				for (auto rule : nonTerminal->val.rules) {
					for (auto arg : *rule->getArgs()) {
						if (arg->isTerminal() || arg->isEpsilon()) continue;

						if (!mNonTerminals.presents(arg->getId())) {
							printf("Referenced non-terminal '%s' is not defined\n", arg->getId().read());
							return false;
						}
					}
				}
			}

			findAllReferences(grammar);

			for (auto nonTerminal : mNonTerminals) {
				if (!nonTerminal->val.references.size() && nonTerminal->key != grammar.getStartTerminal()) {
					printf("Non-terminal '%s' is defined but not used\n", nonTerminal->key.read());
					return false;
				}
			}

			for (auto nonTerminal : mNonTerminals) {
				if (!nonTerminal->val.isProductive()) {
					printf("Non-terminal '%s' is not productive\n", nonTerminal->val.rules.first()->data->id.read());
					return false;
				}
			}

			Map<String, ualni> processed;
			if (mNonTerminals.get(grammar.getStartTerminal()).isLooped(processed, grammar.getStartTerminal())) {
				printf("Note that grammar is looped.\n");
				return false;
			}

			initSymbols(grammar);
		}

		void findNonTerminals(const ContextFreeGrammar& grammar) {
			for (auto rule : *grammar.getRules()) {
				if (!mNonTerminals.presents(rule->getId())) {
					mNonTerminals.put(rule->getId(), {});
				}
				auto nonTerminal = &mNonTerminals.get(rule->getId());
				nonTerminal->rules.append(&rule.data());
			}
		}

		void findAllReferences(const ContextFreeGrammar& grammar) {
			for (auto nonTerminal : mNonTerminals) {
				for (auto rule : nonTerminal->val.rules) {
					for (auto arg : *rule->getArgs()) {
						if (arg->isTerminal() || arg->isEpsilon()) continue;

						NonTerminal* reference = &mNonTerminals.get(arg->getId());
						nonTerminal->val.referencing.put(arg->getId(), reference);
						reference->references.put(nonTerminal->key, &nonTerminal->val);
					}
				}
			}
		}

		void initSymbols(const ContextFreeGrammar& grammar) {
			for (auto nonTerminal : mNonTerminals) {
				mSymbols.append({ nonTerminal->key, false });
				mSymbolLookup.put(nonTerminal->key, SymbolID(mSymbols.size() - 1));

				for (auto rule : nonTerminal->val.rules) {
					for (auto arg : *rule->getArgs()) {
						if (arg->isEpsilon() || arg->isTerminal()) continue;
						if (mTerminals.presents(arg->getId())) continue;
						mTerminals.put(arg->getId(), {});

						mSymbols.append({ nonTerminal->key, true });
						mSymbolLookup.put(nonTerminal->key, SymbolID(mSymbols.size() - 1));
					}
				}
			}
		}

	private:
		Map<String, NonTerminal> mNonTerminals;
		Map<String, bool> mTerminals;
		Buffer<Symbol> mSymbols;
		Map<String, SymbolID> mSymbolLookup;
	};
}
