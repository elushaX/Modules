
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

		private:
			State* mState = nullptr;
			Type mType;
			tAlphabetType mSymbol;
		};

		class State {
			friend FiniteStateAutomation;

			State() = default;

		public:
			void setValue(const tStateType& stateValue) { mStateVal = stateValue; }
			void setAcceptance(bool isAccepting) { mIsAccepting = isAccepting; }

		private:
			Buffer<Transition> mTransitions;
			tStateType mStateVal = tStateType();
			bool mIsAccepting = false;
		};

	private:
		List<State> mStates;
		const State* mStartState = nullptr;

	public:
		FiniteStateAutomation() = default;

		State* addState(const tStateType& state, bool accepting) {
			auto node = mStates.newNode();
			node->data.mIsAccepting = accepting;
			node->data.mState = state;
			mStates.pushBack(node);
			return &node->data;
		}

		void addTransition(State* from, State* to, const tAlphabetType& symbol) {
			from->edges.pushBack(Transition(Transition::SYMBOL, to, symbol));
		}

		void addEpsilonTransition(State* from, State* to) { from->edges.pushBack(Transition(Transition::SYMBOL, to)); }

		void addAnyTransition(State* from, State* to) { from->edges.pushBack(Transition(Transition::ANY, to)); }

		void setStartVertex(State* start) { mStartState = start; }

		[[nodiscard]] State* getStartState() const { return mStartState; }

		[[nodiscard]] bool isValid() const {
			if (!mStartState) {
				return false;
			}
			return true;
		}

		// vertices that are reachable from initial set with no input consumption (E-transitions)
		// does not include initial set
		void findClosureSet(const Buffer<State*>& from, Buffer<State*>& closureSet) const {
			Map<State*, bool> lookup;
			List<State*> workingSet;

			for (auto item : from) {
				workingSet.pushBack(item.data());
			}

			while (workingSet.length()) {
				auto first = workingSet.first()->data;
				closureSet.append(first);

				for (auto edge : first->mTransitions) {
					if (!edge.data().isEpsilon()) continue;
					if (lookup.presents(edge.data().mState)) continue;
					workingSet.pushBack(edge.data().mState);
				}

				workingSet.popFront();
			}
		}

		// vertices that are reachable from initial set with symbol transition
		void findMoveSet(const Buffer<State*>& from, Buffer<State*>& moveSet, tAlphabetType symbol) const {
			Map<State*, bool> lookup;

			for (auto vertex : from) {
				for (auto edge : vertex.mTransitions) {
					if (edge.data().isepsilon()) continue;
					if (!edge.data().isTransition(symbol)) continue;
					if (lookup.presents(edge.data().mState)) continue;
					moveSet.append(edge.data().mState);
					lookup.put(edge.data().mState, {});
				}
			}
		}

		template <typename tAlphabetIterator>
		bool makeDeterministic(const tAlphabetIterator& allSymbols) {
			if (!isValid()) {
				return false;
			}

			struct Group {
				Buffer<State*> states;
				AvlTree<Group*, tAlphabetType> transitions;
				State* newState = nullptr;
				bool accepting = false;
				tStateType stateVal = tStateType();
			};

			struct GroupKey {
				const Group* group;
			};

			Buffer<Group> newStates = { {} };

			// 1) find new states
			Map<GroupKey, bool> lookup;
			List<Group*> workingSet;

			findClosureSet({ getStartState() }, newStates.first().states);

			workingSet.pushBack(&newStates.first());
			while (workingSet.length()) {
				auto group = workingSet.first();

				for (auto symbol : allSymbols) {

					// calculate new possible state
					Group potentialGroup;
					findMoveSet(group.states, potentialGroup.states, symbol);
					if (!potentialGroup.states.size()) continue;

					// find existing or create group
					Group* targetGroup = nullptr;
					auto iter = lookup.presents({ &potentialGroup });
					if (iter) {
						targetGroup = lookup.getSlotVal(iter);
					} else {
						targetGroup = newStates.append({});
						lookup.put({ targetGroup }, {});
					}

					// add transition
					group.transitions.insert(targetGroup, symbol);
				}

				workingSet.popFront();
			}

			// 2) find new states termination values
			// ...

			// 3) transfer
			mStates.removeAll();

			for (auto group : newStates) {
				group.newState = addState();
			}

			for (auto group : newStates) {
				for (auto transition : group.transitions) {
					addTransition(/* ... */);
				}
			}
		}
	};
}