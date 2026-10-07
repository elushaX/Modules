
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

		State* addState(const tStateType& state, bool accepting) {
			auto node = mStates.newNode();
			node->data.mIsAccepting = accepting;
			node->data.mStateVal = state;
			mStates.pushBack(node);
			return &node->data;
		}

		void addTransition(State* from, State* to, const tAlphabetType& symbol) {
			from->mTransitions.append(Transition(Transition::SYMBOL, to, symbol));
			ualni sym = ualni(symbol);
			if (!mAlphabetRange.valid()) {
				mAlphabetRange = { sym, sym + 1 };
			} else {
				if (sym < mAlphabetRange.mBegin) mAlphabetRange.mBegin = sym;
				if (sym + 1 > mAlphabetRange.mEnd) mAlphabetRange.mEnd = sym + 1;
			}
		}

		void addEpsilonTransition(State* from, State* to) { from->mTransitions.append(Transition(Transition::EPSILON, to)); }

		void addAnyTransition(State* from, State* to) { from->mTransitions.append(Transition(Transition::ANY, to)); }

		void setStartState(State* start) { mStartState = start; }

		[[nodiscard]] State* getStartState() const { return mStartState; }

		[[nodiscard]] bool isValid() const {
			if (!mStartState) {
				return false;
			}
			return true;
		}

		[[nodiscard]] ualni numStates() const { return mStates.length(); }

		[[nodiscard]] const List<State>* getStates() const { return &mStates; }

		[[nodiscard]] IterRange<ualni> getAlphabetRange() const { return mAlphabetRange; }

	private:
		typedef Buffer<State*> StatesSet;

		static void statesInsert(StatesSet& set, State* state) {
			if (set.find(state) == -1) {
				set.append(state);
			}
		}

		// Expands initial set with states that are reachable from initial set with no input consumption (E-transitions)
		static void expandSet(StatesSet& set) {
			List<State*> workingSet;

			for (auto state : set) {
				workingSet.pushBack(state.data());
			}

			while (workingSet.length()) {
				auto first = workingSet.first();

				for (auto transition : first->mTransitions) {
					if (!transition->isEpsilon()) continue;
					if (set.find(transition->mState) != -1) continue;
					set.append(transition->mState);
					workingSet.pushBack(transition->mState);
				}

				workingSet.popFront();
			}
		}

		// States that are reachable from initial set with symbol transition
		static void findMoveSet(StatesSet& from, StatesSet& moveSet, tAlphabetType symbol) {
			for (auto state : from) {
				for (auto transition : state.data()->mTransitions) {
					if (transition->isEpsilon()) continue;
					if (!transition->doesConsumes(symbol)) continue;
					statesInsert(moveSet, transition->mState);
				}
			}
		}

		static bool statesEqual(StatesSet& a, StatesSet& b) {
			if (a.size() != b.size()) return false;
			for (auto state : a) {
				if (b.find(state.data()) == -1) return false;
			}
			return true;
		}

	public:
		bool makeDeterministic() {
			if (!isValid()) return false;

			struct Edge {
				ualni from = 0;
				ualni to = 0;
				tAlphabetType symbol = tAlphabetType();
			};

			Buffer<StatesSet> groups;
			Buffer<Edge> edges;

			StatesSet startSet;
			startSet.append(getStartState());
			expandSet(startSet);
			groups.append(startSet);

			// 1) find new states
			List<ualni> workingSet;
			workingSet.pushBack(ualni(0));

			while (workingSet.length()) {
				ualni groupIdx = workingSet.first();
				workingSet.popFront();

				for (auto symbol : getAlphabetRange()) {

					// calculate new possible state
					StatesSet potentialGroup;

					findMoveSet(groups[groupIdx], potentialGroup, tAlphabetType(symbol));
					expandSet(potentialGroup);

					if (!potentialGroup.size()) continue;

					// find existing or create group
					alni existing = -1;
					for (ualni idx = 0; idx < groups.size(); idx++) {
						if (statesEqual(groups[idx], potentialGroup)) {
							existing = (alni) idx;
							break;
						}
					}

					ualni targetGroup = 0;
					if (existing >= 0) {
						targetGroup = (ualni) existing;
					} else {
						targetGroup = groups.size();
						groups.append(potentialGroup);
						workingSet.pushBack(targetGroup);
					}

					edges.append({ groupIdx, targetGroup, tAlphabetType(symbol) });
				}
			}

			// 2) find new states termination values
			Buffer<bool> accepting;
			Buffer<tStateType> stateVals;

			for (auto group : groups) {
				bool isAccepting = false;
				tStateType stateVal = tStateType();

				for (auto state : group.data()) {
					if (state.data()->mIsAccepting) {
						isAccepting = true;
						stateVal = state.data()->mStateVal;
						break;
					}
				}

				accepting.append(isAccepting);
				stateVals.append(stateVal);
			}

			// 3) transfer
			mStates.removeAll();

			Buffer<State*> newStates;
			for (ualni idx = 0; idx < groups.size(); idx++) {
				newStates.append(addState(stateVals[idx], accepting[idx]));
			}

			mStartState = newStates.first();

			for (auto edge : edges) {
				addTransition(newStates[edge.data().from], newStates[edge.data().to], edge.data().symbol);
			}

			return true;
		}
	};
}