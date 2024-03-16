
#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType>
	class RegularAutomata {
	public:
		struct AcceptResult {
			bool accepted = false;
			ualni advancedIdx = 0;
			tStateType state = tStateType();
		};

	public:
		RegularAutomata() = default;

		AcceptResult accept(const tAlphabetType* stream, ualni size) {
			mCurrentState = mStartState;

			ualni advancedIdx = 0;

			while (advancedIdx < size) {
				const tAlphabetType& symbol = *(stream + advancedIdx);

				if (!(symbol >= mSymbolRange.mBegin && symbol < mSymbolRange.mEnd)) {
					return { false, advancedIdx, {} };
				}

				mCurrentState = mTable.get({ (ualni) (symbol - mSymbolRange.mBegin), mCurrentState });

				if (mCurrentState == mStates.size()) {
					return { false, advancedIdx, {} };
				}

				if (mStates[mCurrentState].first) {
					return { true, advancedIdx, mStates[mCurrentState].second };
				}

				advancedIdx++;
			}

			return { false, advancedIdx, {} };
		}

	public:
		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {
			const auto range = automata.getAlphabetRange();
			mSymbolRange = { tAlphabetType(range.mBegin), tAlphabetType(range.mEnd) };

			auto range_len = ualni(mSymbolRange.mEnd - mSymbolRange.mBegin);
			auto sizeX = range_len ? range_len : 1;
			auto sizeY = (ualni) (automata.numStates());

			mTable.reserve({ sizeX, sizeY });
			mTable.assign(automata.numStates());
			mStates.reserve(sizeY);

			ualni idx = 0;
			for (auto state : *automata.getStates()) {
				mStates[idx] = { state->isAccepting(), state->getStateVal() };
				idx++;
			}

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

	private:
		Buffer2D<ualni> mTable;
		Buffer<Pair<bool, tStateType>> mStates;

		ualni mCurrentState = 0;
		ualni mStartState = 0;

		Range<tAlphabetType> mSymbolRange = { 0, 0 };
	};
}
