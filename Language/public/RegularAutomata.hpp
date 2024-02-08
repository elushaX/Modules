
#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType, tStateType tFailedStateVal>
	class RegularAutomata {

		Buffer2D<ualni> mTable;
		Buffer<Pair<bool, tStateType>> mStates;

		ualni mCurrentState = 0;
		ualni mStartState = 0;

		Range<tAlphabetType> mSymbolRange = { 0, 0 };

	public:
		RegularAutomata() = default;

		Pair<tStateType, ualni> accept(const tAlphabetType* stream, ualni size) {
			mCurrentState = mStartState;

			ualni advancedIdx = 0;

			while (advancedIdx < size) {
				tAlphabetType& symbol = *(stream + advancedIdx);

				if (!(symbol >= mSymbolRange.mBegin && symbol < mSymbolRange.mEnd)) {
					return { tFailedStateVal, advancedIdx };
				}

				mCurrentState = mTable.get({ (ualni) (symbol - mSymbolRange.mBegin), mCurrentState });

				if (mStates[mCurrentState].first) {
					return { mStates[mCurrentState].second, advancedIdx };
				}

				advancedIdx++;
			}

			return { tFailedStateVal, advancedIdx };
		}

		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {
			const auto range = automata.getAlphabetRange();
			mSymbolRange = { tAlphabetType(range.mBegin), tAlphabetType(range.mEnd) };

			auto range_len = ualni(mSymbolRange.mEnd - mSymbolRange.mBegin);
			auto sizeX = range_len ? range_len : 1;
			auto sizeY = (ualni) (automata.numStates() + 1);

			mTable.reserve({ sizeX, sizeY });
			mTable.assign(automata.numStates());
			mStates.reserve(sizeY);

			ualni idx = 0;
			for (auto state : *automata.getStates()) {
				mStates[idx] = { state->isAccepting(), state->getStateVal() };
				idx++;
			}

			mStates[automata.numStates()] = { true, tFailedStateVal };

			idx = 0;
			for (auto state : *automata.getStates()) {
				if (&state.data() == automata.getStartState()) {
					mStartState = mCurrentState = idx;
				}
				idx++;
			}

			ualni stateIdx = 0;
			for (auto state : *automata.getStates()) {
				for (auto transition : *state->getTransitions()) {
					ualni stateIdx2 = 0;
					for (auto state2 : *automata.getStates()) {
						if (transition->getState() == &state2.data()) break;
						stateIdx2++;
					}
					auto const code = transition->getSymbol();
					mTable.set({ (ualni) (code - mSymbolRange.mBegin), (ualni) stateIdx }, stateIdx2);
				}
				stateIdx++;
			}
		}
	};
}
