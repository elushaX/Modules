
#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class RegularAutomata {

		static_assert(TypeTraits<tAlphabetType>::isIntegral, "tAlphabetType must be enumerable.");

		Buffer2D<ualni> mTransitions;
		Buffer<tStateType> mStates;
		Range<tAlphabetType> mSymbolRange = { 0, 0 };

		ualni mIter = 0;
		ualni mIterPrev = 0;
		ualni mStart = 0;

	public:
		RegularAutomata() = default;

		auto getStates() const { return &mStates; }
		auto getTransitions() const { return &mTransitions; }
		auto getStart() const { return mStart; }

		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {
			mSymbolRange = { tAlphabetType(automata.getAlphabetRange().mBegin),
											 tAlphabetType(automata.getAlphabetRange().mEnd) };

			auto range_len = ualni(mSymbolRange.mEnd - mSymbolRange.mBegin);
			auto sizeX = range_len ? range_len : 1;
			auto sizeY = (ualni) (automata.numStates() + 1);

			mTransitions.reserve({ sizeX, sizeY });
			mTransitions.assign(automata.numStates());
			mStates.reserve(sizeY);

			ualni idx = 0;
			for (auto state : *automata.getStates()) {
				auto stateVal = state->isAccepting() ? state->getStateVal() : tNoStateVal;
				mStates[idx] = stateVal;
				idx++;
			}

			mStates[automata.numStates()] = tFailedStateVal;

			idx = 0;
			for (auto state : *automata.getStates()) {
				if (&state.data() == automata.getStartState()) {
					mStart = mIter = mIterPrev = idx;
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
					mTransitions.set({ (ualni) (code - mSymbolRange.mBegin), (ualni) stateIdx }, stateIdx2);
				}
				stateIdx++;
			}
		}

		bool isTrapped() { return mStates[mIter] == tFailedStateVal; }

		tStateType move(tAlphabetType symbol) {
			if (symbol >= mSymbolRange.mBegin && symbol < mSymbolRange.mEnd) {
				mIter = mTransitions.get({ (ualni) (symbol - mSymbolRange.mBegin), (ualni) mIter });
			} else {
				mIter = mStates.size() - 1;
			}

			if (mIterPrev == mStart) {
				if (mStates[mIter] == tFailedStateVal) {
					reset();
					return tFailedStateVal;
				} else {
					mIterPrev = mIter;
					return tNoStateVal;
				}
			} else {
				if (mStates[mIter] == tFailedStateVal) {
					if (mStates[mIterPrev] != tNoStateVal) {
						auto out = mStates[mIterPrev];
						reset();
						return out;
					} else {
						reset();
						return tFailedStateVal;
					}
				} else {
					mIterPrev = mIter;
					return tNoStateVal;
				}
			}

			mIterPrev = mIter;
			return mStates[mIter];
		}

		void reset() {
			mIter = mStart;
			mIterPrev = mStart;
		}
	};
}
