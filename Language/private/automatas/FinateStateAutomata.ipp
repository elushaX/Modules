namespace tp {
	template <typename tAlphabetType, typename tStateType>
	auto FiniteStateAutomation<tAlphabetType, tStateType>::addState(const tStateType& state, bool accepting) -> State* {
		auto node = mStates.newNode();
		node->data.mIsAccepting = accepting;
		node->data.mStateVal = state;
		mStates.pushBack(node);
		return &node->data;
	}

	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::addTransition(State* from, State* to, const tAlphabetType& symbol) {
		from->mTransitions.append(Transition(Transition::SYMBOL, to, symbol));
		ualni sym = ualni(symbol);
		if (!mAlphabetRange.valid()) {
			mAlphabetRange = { sym, sym + 1 };
		} else {
			if (sym < mAlphabetRange.mBegin) mAlphabetRange.mBegin = sym;
			if (sym + 1 > mAlphabetRange.mEnd) mAlphabetRange.mEnd = sym + 1;
		}
	}

	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::addEpsilonTransition(State* from, State* to) { from->mTransitions.append(Transition(Transition::EPSILON, to)); }

	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::addAnyTransition(State* from, State* to) { from->mTransitions.append(Transition(Transition::ANY, to)); }

	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::setStartState(State* start) { mStartState = start; }

	template <typename tAlphabetType, typename tStateType>
	auto FiniteStateAutomation<tAlphabetType, tStateType>::getStartState() const -> State* { return mStartState; }

	template <typename tAlphabetType, typename tStateType>
	bool FiniteStateAutomation<tAlphabetType, tStateType>::isValid() const {
		if (!mStartState) {
			return false;
		}
		return true;
	}

	template <typename tAlphabetType, typename tStateType>
	ualni FiniteStateAutomation<tAlphabetType, tStateType>::numStates() const { return mStates.length(); }

	template <typename tAlphabetType, typename tStateType>
	auto FiniteStateAutomation<tAlphabetType, tStateType>::getStates() const -> const List<State>* { return &mStates; }

	template <typename tAlphabetType, typename tStateType>
	IterRange<ualni> FiniteStateAutomation<tAlphabetType, tStateType>::getAlphabetRange() const { return mAlphabetRange; }

	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::statesInsert(StatesSet& set, State* state) {
		if (set.find(state) == -1) {
			set.append(state);
		}
	}

	// Expands initial set with states that are reachable from initial set with no input consumption (E-transitions)
	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::expandSet(StatesSet& set) {
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
	template <typename tAlphabetType, typename tStateType>
	void FiniteStateAutomation<tAlphabetType, tStateType>::findMoveSet(StatesSet& from, StatesSet& moveSet, tAlphabetType symbol) {
		for (auto state : from) {
			for (auto transition : state.data()->mTransitions) {
				if (transition->isEpsilon()) continue;
				if (!transition->doesConsumes(symbol)) continue;
				statesInsert(moveSet, transition->mState);
			}
		}
	}

	template <typename tAlphabetType, typename tStateType>
	bool FiniteStateAutomation<tAlphabetType, tStateType>::statesEqual(StatesSet& a, StatesSet& b) {
		if (a.size() != b.size()) return false;
		for (auto state : a) {
			if (b.find(state.data()) == -1) return false;
		}
		return true;
	}

	template <typename tAlphabetType, typename tStateType>
	bool FiniteStateAutomation<tAlphabetType, tStateType>::makeDeterministic() {
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
}