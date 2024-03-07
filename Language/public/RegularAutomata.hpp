
#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class TransitionMatrix {

		static_assert(TypeTraits<tAlphabetType>::isIntegral, "tAlphabetType must be enumerable.");

		Buffer2D<ualni> mTransitions;
		Buffer<tStateType> mStates;
		Range<tAlphabetType> mSymbolRange = { 0, 0 };

		ualni mIter = 0;
		ualni mIterPrev = 0;
		ualni mStart = 0;

	public:
		TransitionMatrix() = default;

		auto getStates() const { return &mStates; }
		auto getTransitions() const { return &mTransitions; }
		auto getStart() const { return mStart; }

		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& dfa) {
			// mSymbolRange = { dfa.getRange().first(), dfa.getRange().last() };
			ASSERT(0);

			/*
			auto range_len = ualni(mSymbolRange.mEnd - mSymbolRange.mBegin);
			auto sizeX = range_len ? range_len : 1;
			auto sizeY = (ualni) (dfa.nVertices() + 1);

			mTransitions.reserve({ sizeX, sizeY });
			mTransitions.assign(dfa.nVertices());
			mStates.reserve(sizeY);

			ualni idx = 0;
			for (auto vertex : dfa.mVertices) {
				auto state = vertex.data().termination ? vertex.data().state : tNoStateVal;
				mStates[idx] = state;
				idx++;
			}

			mStates[dfa.nVertices()] = tFailedStateVal;

			idx = 0;
			for (auto vertex : dfa.mVertices) {
				if (&vertex.data() == dfa.mStart) {
					mStart = mIter = mIterPrev = idx;
				}
				idx++;
			}

			ualni vertexIdx = 0;
			for (auto vertex : dfa.mVertices) {
				for (auto edge : vertex.data().edges) {
					ualni vertex2Idx = 0;
					for (auto vertex2 : dfa.mVertices) {
						if (edge.data().vertex == &vertex2.data()) break;
						vertex2Idx++;
					}
					auto const code = edge.data().transition_code;
					mTransitions.set({ (ualni) (code - mSymbolRange.mBegin), (ualni) vertexIdx }, vertex2Idx);
				}
				vertexIdx++;
			}

			 */
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
