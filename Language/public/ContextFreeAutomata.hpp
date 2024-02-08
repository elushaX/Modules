
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

	public:
		struct StackItem {
			ualni state = 0;
			tAlphabetType symbol;
			Buffer<StackItem*> leafs;
		};

		struct AcceptResult {
			bool accepted = false;
			ualni advancedIdx = 0;
			const StackItem* ast = nullptr;
		};

	public:
		ContextFreeAutomata() = default;

		AcceptResult accept(const tAlphabetType* stream, ualni size) {
			mCurrentState = mStartState;
			mStack.append(&mItems.append({ mCurrentState, {}, {} }));

			ualni advancedIdx = 0;
			while (advancedIdx < size) {
				tAlphabetType& symbol = *(stream + advancedIdx);

				if (!(symbol >= mRange.mBegin && symbol < mRange.mEnd)) {
					return { false, advancedIdx, nullptr };
				}

				const Action& action = mTable.get({ ualni(symbol - mRange.mBegin), mCurrentState });

				if (action.type == Action::TRAP) {
					return { false, advancedIdx, nullptr };
				}

				if (action.type == Action::SHIFT) {
					mStack.last()->symbol = symbol;
					mStack.append(&mItems.append({ mCurrentState, {}, {} }));
					mCurrentState = action.num;
				}

				if (mTable.get({ 0, mCurrentState }).type == Action::REDUCE) {
					StackItem* newItem = &mItems.append({});
					for (auto iter : Range<ualni>(action.num)) {
						newItem->leafs.append(mStack.last());
						mCurrentState = mStack.last()->state;
						mStack.pop();
					}

					if (!mStack.size()) {
						if (advancedIdx == size) {
							return { true, advancedIdx, newItem };
						} else {
							return { false, advancedIdx, {} };
						}
					}

					mStack.append(&mItems.append({ mCurrentState, {}, {} }));
				}

				advancedIdx++;
			}

			return { false, advancedIdx, nullptr };
		}

	public:
		typedef FiniteStateAutomation<tAlphabetType, tStateType> Automata;
		typedef Automata::State AutomataState;

		void construct(const Automata& automata) {
			mRange = automata.getAlphabetRange();

			const ualni numStates = automata.numStates();
			const ualni numSymbols = mRange.idxDiff();

			mTable.reserve({ numSymbols, numStates });
			mTable.assign(Action{ Action::TRAP, 0 });

			Map<const AutomataState*, ualni> states;
			ualni stateIndex = 0;
			for (auto state : *automata.getStates()) {
				states.put(state.data(), { stateIndex });
				stateIndex++;
			}

			stateIndex = 0;
			for (auto state : *automata.getStates()) {
				if (state.data() == automata.getStartState()) {
					mStartState = stateIndex;
				}

				if (state->isAccepting()) {
					ASSERT(state->getTransitions().size() == 0)
					for (auto symbolIndex : Range<ualni>(numSymbols)) {
						mTable.set({ stateIndex, symbolIndex }, { Action::REDUCE, state->getStateVal() });
					}
				} else {
					for (auto transition : *state->getTransitions()) {
						ualni symbolIndex = ualni(transition->getSymbol()) - mRange.mBegin;
						ualni targetStateIndex = states.get(transition->getState());
						mTable.set({ stateIndex, symbolIndex }, { Action::SHIFT, targetStateIndex });
					}
				}

				stateIndex++;
			}
		}

	private:
		Buffer2D<Action> mTable;

		Buffer<StackItem> mItems;
		Buffer<StackItem*> mStack;

		ualni mStartState = 0;
		ualni mCurrentState = 0;

		Range<tAlphabetType> mRange = { 0, 0 };
	};
}
