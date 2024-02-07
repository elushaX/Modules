
#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType>
	class ContextFreeAutomata {

	public:
		ContextFreeAutomata() = default;

		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {}
	};
}
