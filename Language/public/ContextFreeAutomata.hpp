

#pragma once

#include "Strings.hpp"
#include "Automata.hpp"
#include "Buffer2D.hpp"
#include "ContextFreeCompiler.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType>
	class ContextFreeAutomata {

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
			mItems.removeAll();
			mStateStack.clear();
			mValueStack.clear();

			auto newItem = [&](tAlphabetType symbol) -> StackItem* {
				auto node = mItems.newNode();
				node->data.symbol = symbol;
				node->data.leafs.clear();
				mItems.pushBack(node);
				return &node->data;
			};

			mStateStack.append(ualni(0));

			ualni advancedIdx = 0;

			while (true) {
				ualni state = mStateStack.last();
				ualni look = (advancedIdx < size) ? (ualni(stream[advancedIdx].id) - mNonTerminalCount) : mTerminalCount;

				const ContextFreeCompiler::Action& action = mActionTable.get({ state, look });

				if (action.type == ContextFreeCompiler::Action::SHIFT) {
					mStateStack.append(action.num);
					mValueStack.append(newItem(stream[advancedIdx]));
					advancedIdx++;

				} else if (action.type == ContextFreeCompiler::Action::REDUCE) {
					ualni rule = action.num;
					ualni pop = mRulePopCount[rule];
					alni lhs = mRuleLhs[rule];

					StackItem* parent = newItem(mNonTerminalSymbols[(ualni) lhs]);

					for (ualni i = 0; i < pop; i++) {
						parent->leafs.append(mValueStack.last());
						mValueStack.pop();
						mStateStack.pop();
					}

					for (ualni i = 0; i < parent->leafs.size() / 2; i++) {
						auto tmp = parent->leafs[i];
						parent->leafs[i] = parent->leafs[parent->leafs.size() - 1 - i];
						parent->leafs[parent->leafs.size() - 1 - i] = tmp;
					}

					ualni gotoState = (ualni) mGotoTable.get({ mStateStack.last(), (ualni) lhs });
					mStateStack.append(gotoState);
					mValueStack.append(parent);

				} else if (action.type == ContextFreeCompiler::Action::ACCEPT) {
					return { true, advancedIdx, mValueStack.size() ? mValueStack.last() : nullptr };
				} else {
					return { false, advancedIdx, nullptr };
				}
			}
		}

	public:
		void construct(const ContextFreeCompiler& compiler) {
			mNonTerminalCount = compiler.getNonTerminals()->size();
			mTerminalCount = compiler.getTerminals()->size();
			mStartSymbol = compiler.getStartSymbol();

			mNonTerminalSymbols.clear();
			for (auto sym : *compiler.getNonTerminals()) {
				mNonTerminalSymbols.append(sym.data());
			}

			mRuleLhs.clear();
			for (auto lhs : *compiler.getRuleLhs()) {
				mRuleLhs.append(lhs.data());
			}

			mRulePopCount.clear();
			for (auto pop : *compiler.getRulePopCount()) {
				mRulePopCount.append(pop.data());
			}

			ualni S = compiler.getActionTable()->size().x;
			ualni N = mNonTerminalCount;
			ualni T = mTerminalCount;

			mActionTable.reserve({ S, T + 1 });
			for (ualni s = 0; s < S; s++) {
				for (ualni t = 0; t <= T; t++) {
					mActionTable.set({ s, t }, compiler.getActionTable()->get({ s, t }));
				}
			}

			mGotoTable.reserve({ S, N });
			for (ualni s = 0; s < S; s++) {
				for (ualni n = 0; n < N; n++) {
					mGotoTable.set({ s, n }, compiler.getGotoTable()->get({ s, n }));
				}
			}
		}

	private:
		Buffer2D<ContextFreeCompiler::Action> mActionTable;
		Buffer2D<alni> mGotoTable;
		Buffer<alni> mRuleLhs;
		Buffer<ualni> mRulePopCount;
		Buffer<ContextFreeCompiler::SymbolVal> mNonTerminalSymbols;

		Buffer<ualni> mStateStack;
		Buffer<StackItem*> mValueStack;
		List<StackItem> mItems;

		tAlphabetType mStartSymbol = tAlphabetType();

		ualni mNonTerminalCount = 0;
		ualni mTerminalCount = 0;
	};
}
