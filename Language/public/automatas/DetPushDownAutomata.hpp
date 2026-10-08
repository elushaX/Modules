

#pragma once

#include "Strings.hpp"
#include "FinateStateAutomata.hpp"
#include "Buffer2D.hpp"
#include "../compilers/ContextFreeCompiler.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType>
	class DetPushDownAutomata {

	public:
		struct StackItem {
			ualni state = 0;
			tAlphabetType symbol;
			alni rule = -1; // reduce rule index that produced this node (-1 for shifted terminals)
			Buffer<StackItem*> leafs;
		};

		struct AcceptResult {
			bool accepted = false;
			ualni advancedIdx = 0;
			const StackItem* ast = nullptr;
		};

	public:
		DetPushDownAutomata() = default;

		AcceptResult accept(const tAlphabetType* stream, ualni size);

	public:
		void construct(const ContextFreeCompiler& compiler);

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

#include "automatas/DetPushDownAutomata.ipp"
