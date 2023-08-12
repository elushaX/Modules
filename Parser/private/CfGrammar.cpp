
#include "NewPlacement.hpp"
#include "CfGrammar.hpp"

#include "Timing.hpp"

using namespace tp;

void CfGrammar::generateSentences(List<Sentence>& out) {
	constexpr ualni maxTime = 1000;
	constexpr ualni maxSentences = 200;
	constexpr ualni maxQueue = 1000000;

	List<Sentence> queue;
	Timer timer(maxTime);

	// add start production
	Sentence start;
	start.terms.pushBack( { startTerminal, false } );
	queue.pushBack(start);

	PASS:

	auto const sentential = &queue.first()->data;
	bool isSentence = true;
	ualni termIdx = 0;
	for (auto term : sentential->terms) {
		if (term->terminal) {
			termIdx++;
			continue;
		}

		auto nonTerminal = &mNonTerminals.get(term->id);
		for (auto production : nonTerminal->rules) {
			queue.pushBack(*sentential);
			auto copy = &queue.last()->data;

			auto appendTerm = copy->terms.findIdx(termIdx);
			auto deleteTerm = appendTerm;
			for (auto arg : production->args) {
				if (arg->isEpsilon) continue;
				auto newTerm = copy->terms.newNode({ arg->id, arg->isTerminal });
				copy->terms.attach(newTerm, appendTerm);
				appendTerm = newTerm;
			}
			copy->terms.removeNode(deleteTerm);
		}

		isSentence = false;
		termIdx++;
	}

	if (isSentence) {
		auto exists = false;
		for (auto iter : out) {
			if (iter->terms == sentential->terms) {
				exists = true;
				break;
			}
		}
		if (!exists) out.pushBack(*sentential);
	}

	queue.popFront();

	bool stop = false;
	stop |= !queue.length();
	stop |= timer.isTimeout();
	stop |= queue.length() > maxQueue;
	stop |= out.length() > maxSentences;

	if (!stop) goto PASS;
}

void CfGrammar::printSentence(Sentence& in) {
	printf("[");
	for (auto term : in.terms) {
		printf(" %s", term->id.read());
	}
	printf(" ]\n");
}

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
				} else if (!arg->isEpsilon) {
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

	List<String> remove;
	for (auto nonTerminal : mNonTerminals) {
		if (!nonTerminal->val.references.size() && nonTerminal->key != startTerminal) {
			printf("Non-terminal '%s' is defined but not used\n", nonTerminal->key.read());
			remove.pushBack(nonTerminal->key);
		}
	}

	for (auto rem : remove) {
		auto nonTerminal = &mNonTerminals.get(rem.data());
		for (auto referencing : nonTerminal->referencing) {
			referencing->val->references.remove(nonTerminal->rules.first()->data->id);
		}
		for (auto rule : nonTerminal->rules) {
			rules.removeNode(rules.find(*rule.data()));
		}
	}

	for (auto nonTerminal : mNonTerminals) {
		if (!nonTerminal->val.isProductive()) {
			printf("Non-terminal '%s' is not productive\n", nonTerminal->val.rules.first()->data->id.read());
			return false;
		}
	}

	Map<String, ualni> processed;
	if (mNonTerminals.get(startTerminal).isLooped(processed, startTerminal)) {
		printf("Note that grammar is looped.\n");
	}

	return true;
}