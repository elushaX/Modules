
#pragma once

#include "Utils.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "Tree.hpp"

namespace tp {

	// Non-Deterministic Finite-State Automata
	template <typename tAlphabetType, typename tStateType>
	class FiniteStateAutomation {
	public:
		struct State;

	public:
		class Transition {
			friend FiniteStateAutomation;

		public:
			enum Type { ANY, EPSILON, SYMBOL };

		public:
			Transition(Type type, State* state, tAlphabetType symbol = tAlphabetType()) {
				mState = state;
				mType = type;
				mSymbol = symbol;
			}

			[[nodiscard]] bool isTransition(const tAlphabetType& symbol) const {
				return (mType == ANY || mType == EPSILON) || (mSymbol == symbol);
			}

			[[nodiscard]] bool doesConsumes(const tAlphabetType& symbol) const {
				return (mType == ANY || (mType == SYMBOL && mSymbol == symbol));
			}

			[[nodiscard]] bool isEpsilon() const { return mType == EPSILON; }

			const State* getState() const { return mState; }
			const tAlphabetType& getSymbol() const { return mSymbol; }

		private:
			State* mState = nullptr;
			Type mType;
			tAlphabetType mSymbol;
		};

		class State {
			friend FiniteStateAutomation;

		public:
			State() = default;

		public:
			void setValue(const tStateType& stateValue) { mStateVal = stateValue; }
			void setAcceptance(bool isAccepting) { mIsAccepting = isAccepting; }
			[[nodiscard]] bool isAccepting() const { return mIsAccepting; }
			const tStateType& getStateVal() const { return mStateVal; }
			[[nodiscard]] const Buffer<Transition>* getTransitions() const { return &mTransitions; }

		private:
			Buffer<Transition> mTransitions{};
			tStateType mStateVal = tStateType();
			bool mIsAccepting = false;
		};

	private:
		List<State> mStates;
		State* mStartState = nullptr;
		IterRange<ualni> mAlphabetRange = { 0, 0 };

	public:
		FiniteStateAutomation() = default;

		State* addState(const tStateType& state, bool accepting);

		void addTransition(State* from, State* to, const tAlphabetType& symbol);

		void addEpsilonTransition(State* from, State* to);

		void addAnyTransition(State* from, State* to);

		void setStartState(State* start);

		[[nodiscard]] State* getStartState() const;

		[[nodiscard]] bool isValid() const;

		[[nodiscard]] ualni numStates() const;

		[[nodiscard]] const List<State>* getStates() const;

		[[nodiscard]] IterRange<ualni> getAlphabetRange() const;

	private:
		typedef Buffer<State*> StatesSet;

		static void statesInsert(StatesSet& set, State* state);

		// Expands initial set with states that are reachable from initial set with no input consumption (E-transitions)
		static void expandSet(StatesSet& set);

		// States that are reachable from initial set with symbol transition
		static void findMoveSet(StatesSet& from, StatesSet& moveSet, tAlphabetType symbol);

		static bool statesEqual(StatesSet& a, StatesSet& b);

	public:
		bool makeDeterministic();
	};
}

#include "automatas/FinateStateAutomata.ipp"
