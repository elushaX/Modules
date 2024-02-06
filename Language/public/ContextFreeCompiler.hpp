
#pragma once

#include "Automata.hpp"
#include "Grammar.hpp"

namespace tp {

	class ContextFreeCompiler {
		typedef ContextFreeGrammar Grammar;

	public:
		// Terminals or non terminals
		struct Alphabet {
			String mId;
			bool mIsTerminal = false;
		};

		struct Item {
			Grammar::Rule mRule;
			ualni mAdvanceIdx;
		};

		typedef NFA<Alphabet, Item> NFA;
		typedef typename NFA::Vertex State;

	public:
		void compile(const Grammar& in, NFA& out) { nfa = &out; }

	private:
		NFA* nfa = nullptr;
	};
}
