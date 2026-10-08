
#pragma once

#include "../automatas/FinateStateAutomata.hpp"
#include "../grammars/RegularGrammar.hpp"

namespace tp {

	class ContextFreeCompiler {
	public:
		struct SymbolVal {
			SymbolVal() = default;

			SymbolVal(ualni aId, ualni aStart, ualni aLen) {
				id = aId;
				start = aStart;
				len = aLen;
			};

			SymbolVal(ualni val) { id = val; }

			operator ualni() const { return id; }

			bool operator==(const SymbolVal& in) const { return in.id == id; }
			SymbolVal& operator=(const SymbolVal& in) = default;

			ualni id = 0;
			ualni start = 0;
			ualni len = 0;
		};

		struct Item {
			const ContextFreeGrammar::Rule* mRule = nullptr;
			ualni mAdvanceIdx = 0;
			ualni numArgs() const { return mRule ? mRule->getArgs()->size() : 0; }
		};

		struct Symbol {
			String mId;
			bool mIsTerminal = false;
		};

		struct Action {
			enum Type { ERROR, SHIFT, REDUCE, ACCEPT } type = ERROR;
			ualni num = 0; // target state (shift) or rule index (reduce)
		};

	private:
		struct NonTerminal {
			Buffer<ContextFreeGrammar::Rule*> rules;
			Map<String, NonTerminal*> references;
			Map<String, NonTerminal*> referencing;

		public:
			[[nodiscard]] bool isProductive() const {
				for (auto rule : rules) {
					if (rule->isProductive()) return true;
				}
				return false;
			}

			[[nodiscard]] bool isLooped(Map<String, ualni>& processed, const String& id) const {
				for (auto ref : referencing) {
					if (processed.presents(ref->key)) return true;
				}
				processed.put(id, {});
				for (auto ref : referencing) {
					if (ref->val->isLooped(processed, ref->key)) return true;
				}
				return false;
			}
		};

		struct LrItem {
			ualni rule = 0; // 0 is the augmented start rule
			ualni dot = 0;
			ualni look = 0; // terminal index (local), T == EOF
		};

		struct Edge {
			ualni from = 0;
			alni symbol = 0; // global symbol index, -1 for none
			ualni to = 0;
		};

	public:
		bool compile(const ContextFreeGrammar& grammar);

		[[nodiscard]] const Buffer<Symbol>* getSymbols() const;
		[[nodiscard]] SymbolVal getSymbolId(const String& name) const;
		[[nodiscard]] const Buffer<SymbolVal>* getTerminals() const;
		[[nodiscard]] const Buffer<SymbolVal>* getNonTerminals() const;
		[[nodiscard]] SymbolVal getStartSymbol() const;

		[[nodiscard]] const Buffer2D<Action>* getActionTable() const;
		[[nodiscard]] const Buffer2D<alni>* getGotoTable() const;
		[[nodiscard]] const Buffer<alni>* getRuleLhs() const;
		[[nodiscard]] const Buffer<ualni>* getRulePopCount() const;

	private:
		bool init(const ContextFreeGrammar& grammar);

		void findNonTerminals(const ContextFreeGrammar& grammar);

		void findAllReferences(const ContextFreeGrammar& grammar);

		static bool argIsEpsilon(const ContextFreeGrammar::Arg& arg);

		bool buildTable(const ContextFreeGrammar& grammar);

	private:
		Map<String, NonTerminal> mNonTerminals;
		Buffer<Symbol> mSymbols;
		Map<String, SymbolVal> mSymbolLookup;

		Buffer<SymbolVal> mNonTerminalSymbols;
		Buffer<SymbolVal> mTerminalSymbols;
		Map<String, ualni> mNonTerminalIdx;
		Map<String, ualni> mTerminalIdx;

		SymbolVal mStartSymbol = SymbolVal(0);

		Buffer2D<Action> mActionTable;
		Buffer2D<alni> mGotoTable;
		Buffer<alni> mRuleLhs;
		Buffer<ualni> mRulePopCount;
	};
}

#include "compilers/ContextFreeCompiler.ipp"
