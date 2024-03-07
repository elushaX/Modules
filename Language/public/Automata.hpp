
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

		public:
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
		State* mStartState = nullptr;

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
		}

		void addEpsilonTransition(State* from, State* to) { from->mTransitions.append(Transition(Transition::SYMBOL, to)); }

		void addAnyTransition(State* from, State* to) { from->mTransitions.append(Transition(Transition::ANY, to)); }

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
			Map<alni, bool> lookup;
			List<State*> workingSet;

			for (auto item : from) {
				workingSet.pushBack(item.data());
			}

			while (workingSet.length()) {
				auto first = workingSet.first()->data;
				closureSet.append(first);
				lookup.put((alni) first, {});

				for (auto edge : first->mTransitions) {
					if (!edge.data().isEpsilon()) continue;
					if (lookup.presents((alni) edge.data().mState)) continue;
					workingSet.pushBack(edge.data().mState);
				}

				workingSet.popFront();
			}
		}

		// vertices that are reachable from initial set with symbol transition
		void findMoveSet(const Buffer<State*>& from, Buffer<State*>& moveSet, tAlphabetType symbol) const {
			Map<alni, bool> lookup;

			for (auto vertex : from) {
				for (auto edge : vertex->mTransitions) {
					if (edge.data().isEpsilon()) continue;
					if (!edge.data().isTransition(symbol)) continue;
					if (lookup.presents((alni) edge.data().mState)) continue;
					moveSet.append(edge.data().mState);
					lookup.put((alni) edge.data().mState, {});
				}
			}
		}

		template <typename tAlphabetIterator>
		bool makeDeterministic(const tAlphabetIterator& allSymbols) {
			if (!isValid()) return false;

			typedef Buffer<State*> Group;

			struct GroupKey {
				const Group* group;
				static ualni hash(GroupKey key) { return 0; }
				bool operator==(const GroupKey& key) const { return false; }
			};

			struct GroupInfo {
				Group* group = nullptr;
				AvlTree<AvlNumericKey<alni>, tAlphabetType> transitions;
				State* newState = nullptr;
				bool accepting = false;
				tStateType stateVal = tStateType();
			};

			Buffer<Group> groups = { {} };
			Map<GroupKey, GroupInfo, DefaultAllocator, GroupKey::hash> groupInfos;

			findClosureSet({ getStartState() }, groups.first());
			groupInfos.put({ &groups.first() }, { &groups.first() });

			// 1) find new states
			List<Group*> workingSet;
			workingSet.pushBack(&groups.first());

			while (workingSet.length()) {
				Group* group = workingSet.first()->data;
				GroupInfo* info = &groupInfos.get({ group });

				for (auto symbol : allSymbols) {

					// calculate new possible state
					Group potentialGroupTmp;
					Group potentialGroup;

					findMoveSet(*group, potentialGroupTmp, symbol);
					findClosureSet(potentialGroupTmp, potentialGroup);

					if (!potentialGroup.size()) continue;

					// find existing or create group
					Group* targetGroup = nullptr;
					auto iter = groupInfos.presents({ &potentialGroup });
					if (iter) {
						targetGroup = groupInfos.getSlotVal(iter).group;
					} else {
						targetGroup = &groups.append(potentialGroup);
						groupInfos.put({ targetGroup }, { targetGroup });
						workingSet.pushBack(targetGroup);
					}

					// assert transition is added
					info->transitions.insert((alni) targetGroup, symbol);
				}

				workingSet.popFront();
			}

			// 2) find new states termination values
			for (auto group : groupInfos) {
				GroupInfo* info = &group->val;
				bool accepting = false;
				for (auto item : *info->group) {
					if (item->mIsAccepting) {
						if (accepting) return false;
						accepting = true;
						info->accepting = true;
						info->stateVal = item->mStateVal;
					}
				}
				if (!accepting) {
					info->accepting = false;
					info->stateVal = info->group->first()->mStateVal;
				}
			}

			// 3) transfer
			mStates.removeAll();

			// create states
			for (auto group : groupInfos) {
				group->val.newState = addState(group->val.stateVal, group->val.accepting);
			}

			// create transitions
			for (auto group : groupInfos) {
				auto functor = [&](AvlNumericKey<alni> targetGroupKey, tAlphabetType symbol) {
					GroupInfo* targetGroup = &groupInfos.get({ (Group*) targetGroupKey.val });
					addTransition(group->val.newState, targetGroup->newState, symbol);
				};
				group->val.transitions.forEach(functor);
			}

			return true;
		}
	};
}