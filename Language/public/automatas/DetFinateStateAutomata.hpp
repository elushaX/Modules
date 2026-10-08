
#pragma once

#include "Strings.hpp"
#include "FinateStateAutomata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType>
	class DetFinateStateAutomata {
	public:
		struct AcceptResult {
			bool accepted = false;
			ualni advancedIdx = 0;
			tStateType state = tStateType();
		};

	public:
		DetFinateStateAutomata() = default;

		AcceptResult accept(const tAlphabetType* stream, ualni size);

		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata);

	private:
		Buffer2D<ualni> mTable;
		Buffer<std::pair<bool, tStateType>> mStates;

		ualni mCurrentState = 0;
		ualni mStartState = 0;

		IterRange<tAlphabetType> mSymbolRange = { 0, 0 };
	};
}

#include "automatas/DetFinateStateAutomata.ipp"
