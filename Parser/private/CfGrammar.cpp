
#include "NewPlacement.hpp"
#include "CfGrammar.hpp"

using namespace tp;

bool CfGrammar::compile() {

	if (!rules.length()) {
		printf("Grammar must have at leas one rule\n");
		return false;
	}

	if (startTerminal == String()) {
		printf("Using first rule's non-terminal as grammar's root. Define explicitly - 'Start : id'\n");
		startTerminal = rules.first()->data.id;
	}

	// find all rules
	for (auto rule : rules) {

		if (!rule->args.length()) {
			printf("Rule must have at leas one terminal or non-terminal. See rule '%s'\n", rule->id.read());
			return false;
		}

		if (!mNonTerminals.presents(rule->id)) {
			mNonTerminals.put(rule->id, {});
		}
		auto nonTerminal = &mNonTerminals.get(rule->id);

		nonTerminal->rules.pushBack(&rule.data());
	}

	for (auto nonTerminal : mNonTerminals) {
		for (auto rule : nonTerminal->val.rules) {
			for (auto arg : rule->args) {

				if (arg->isTerminal) {
					mTerminals.put(rule->id, {});
				} else {
					auto idx = mNonTerminals.presents(arg->id);
					if (!idx) {
						printf("Referenced non-terminal '%s' is not defined\n", arg->id.read());
						return false;
					}
					auto reference = &mNonTerminals.getSlotVal(idx);
					reference->references.put(nonTerminal->key, &nonTerminal->val);
					nonTerminal->val.referencing.put(arg->id, reference);
				}
			}
		}
	}

	for (auto nonTerminal : mNonTerminals) {
		if (!nonTerminal->val.references.size() && nonTerminal->key != startTerminal) {
			printf("Non-terminal '%s' is defined but not used\n", nonTerminal->key.read());
		}
	}

	return true;
}