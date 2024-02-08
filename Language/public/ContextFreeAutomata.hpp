
#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType>
	class ContextFreeAutomata {

		struct Action {
			enum Type { SHIFT, REDUCE, TRAP } type = TRAP;
			ualni num = 0; // state to shift (shift action) or pop count (reduce action)
		};

		Buffer2D<Action> mTable;

	public:
		ContextFreeAutomata() = default;

		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {}
	};
}
