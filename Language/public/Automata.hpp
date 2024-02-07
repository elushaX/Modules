
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
		Range<ualni> mAlphabetRange = { ENV_UALNI_MAX, ENV_UALNI_MIN };

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
			if (mAlphabetRange.mBegin < ualni(symbol)) mAlphabetRange.mBegin = ualni(symbol);
			if (mAlphabetRange.mEnd > ualni(symbol)) mAlphabetRange.mEnd = ualni(symbol);
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

		[[nodiscard]] ualni numStates() const { return mStates.length(); }

		[[nodiscard]] const List<State>* getStates() const { return &mStates; }

		[[nodiscard]] Range<ualni> getAlphabetRange() const { return mAlphabetRange; }

	private:
		typedef AvlTree<AvlNumericKey<State*>, bool> StatesSet;

		// Expands initial set with states that are reachable from initial set with no input consumption (E-transitions)
		static void expandSet(StatesSet& set) {
			List<State*> workingSet;

			set.forEach([&](AvlNumericKey<State*>& key, bool) { workingSet.pushBack(key.val); });

			while (workingSet.length()) {
				auto first = workingSet.first()->data;
				set.insert(first, {});

				for (auto transition : first->mTransitions) {
					if (!transition->isEpsilon()) continue;
					if (set.find(transition->mState)) continue;
					workingSet.pushBack(transition->mState);
				}

				workingSet.popFront();
			}
		}

		// States that are reachable from initial set with symbol transition
		static void findMoveSet(StatesSet& from, StatesSet& moveSet, tAlphabetType symbol) {
			from.forEach([&](AvlNumericKey<State*>& key, bool) {
				for (auto transition : key.val->mTransitions) {
					if (transition->isEpsilon()) continue;
					if (!transition->isTransition(symbol)) continue;
					if (moveSet.find(transition->mState)) continue;
					moveSet.insert(transition->mState, {});
				}
			});
		}

	public:
		bool makeDeterministic() {
			if (!isValid()) return false;

			struct GroupKey {
				const StatesSet* group;
				static ualni hash(GroupKey key) { return 0; }
				bool operator==(const GroupKey& key) const { return false; }
			};

			struct GroupInfo {
				StatesSet* group = nullptr;
				AvlTree<AvlNumericKey<StatesSet*>, tAlphabetType> transitions;
				State* newState = nullptr;
				bool accepting = false;
				tStateType stateVal = tStateType();
			};

			Buffer<StatesSet> groups = { {} };
			Map<GroupKey, GroupInfo, DefaultAllocator, GroupKey::hash> groupInfos;

			groups.first().insert(getStartState(), false);

			expandSet(groups.first());

			groupInfos.put({ &groups.first() }, { &groups.first() });

			// 1) find new states
			List<StatesSet*> workingSet;
			workingSet.pushBack(&groups.first());

			while (workingSet.length()) {
				StatesSet* group = workingSet.first()->data;
				GroupInfo* info = &groupInfos.get({ group });

				for (auto symbol : getAlphabetRange()) {

					// calculate new possible state
					StatesSet potentialGroup;

					findMoveSet(*group, potentialGroup, symbol);
					expandSet(potentialGroup);

					if (!potentialGroup.size()) continue;

					// find existing or create group
					StatesSet* targetGroup = nullptr;
					auto iter = groupInfos.presents({ &potentialGroup });
					if (iter) {
						targetGroup = groupInfos.getSlotVal(iter).group;
					} else {
						targetGroup = &groups.append(potentialGroup);
						groupInfos.put({ targetGroup }, { targetGroup });
						workingSet.pushBack(targetGroup);
					}

					// assert transition is added
					info->transitions.insert(targetGroup, symbol);
				}

				workingSet.popFront();
			}

			// 2) find new states termination values
			for (auto group : groupInfos) {
				GroupInfo* info = &group->val;
				ualni accepting = 0;

				info->group->forEach([&](AvlNumericKey<State*>& key, bool) {
					if (key.val->mIsAccepting) {
						accepting++;
						info->accepting = true;
						info->stateVal = key.val->mStateVal;
					}
				});

				if (!accepting) {
					info->accepting = false;
					info->stateVal = info->group->head()->key.val->mStateVal;
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
				auto functor = [&](AvlNumericKey<StatesSet*> targetGroupKey, tAlphabetType symbol) {
					GroupInfo* targetGroup = &groupInfos.get({ (StatesSet*) targetGroupKey.val });
					addTransition(group->val.newState, targetGroup->newState, symbol);
				};
				group->val.transitions.forEach(functor);
			}

			return true;
		}
	};
}