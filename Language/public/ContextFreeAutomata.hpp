
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

				const Action& action = mTable.get({ ualni(symbol), mCurrentState });

				switch (action.type) {
					case Action::TRAP: return { false, advancedIdx, nullptr };
					case Action::REDUCE:
						{
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
							break;
						}
					case Action::SHIFT:
						{
							mStack.last()->symbol = symbol;
							mStack.append(&mItems.append({ mCurrentState, {}, {} }));
							mCurrentState = action.num;
							break;
						}
				}

				advancedIdx++;
			}

			return { false, advancedIdx, nullptr };
		}

	public:
		void construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {}

	private:
		Buffer2D<Action> mTable;

		Buffer<StackItem> mItems;
		Buffer<StackItem*> mStack;

		ualni mStartState = 0;
		ualni mCurrentState = 0;
	};
}
