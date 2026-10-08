namespace tp {
	template <typename tAlphabetType, typename tStateType>
	auto DetFinateStateAutomata<tAlphabetType, tStateType>::accept(const tAlphabetType* stream, ualni size) -> AcceptResult {
		mCurrentState = mStartState;

		ualni advancedIdx = 0;
		bool hasAccept = false;
		ualni acceptIdx = 0;
		tStateType acceptState = tStateType();

		while (advancedIdx < size) {
			const tAlphabetType& symbol = *(stream + advancedIdx);

			if (!(symbol >= mSymbolRange.mBegin && symbol < mSymbolRange.mEnd)) {
				break;
			}

			mCurrentState = mTable.get({ (ualni) (symbol - mSymbolRange.mBegin), mCurrentState });

			if (mCurrentState == mStates.size()) {
				break;
			}

			advancedIdx++;

			if (mStates[mCurrentState].first) {
				hasAccept = true;
				acceptIdx = advancedIdx;
				acceptState = mStates[mCurrentState].second;
			}
		}

		if (hasAccept) {
			return { true, acceptIdx, acceptState };
		}

		return { false, advancedIdx, {} };
	}

	template <typename tAlphabetType, typename tStateType>
	void DetFinateStateAutomata<tAlphabetType, tStateType>::construct(const FiniteStateAutomation<tAlphabetType, tStateType>& automata) {
		const auto range = automata.getAlphabetRange();
		mSymbolRange = { tAlphabetType(range.mBegin), tAlphabetType(range.mEnd) };

		auto range_len = ualni(mSymbolRange.mEnd - mSymbolRange.mBegin);
		auto sizeX = range_len ? range_len : 1;
		auto sizeY = (ualni) (automata.numStates());

		mTable.reserve({ sizeX, sizeY });
		mTable.assign(automata.numStates());
		mStates.reserve(sizeY);

		ualni idx = 0;
		for (auto state : *automata.getStates()) {
			mStates[idx] = { state->isAccepting(), state->getStateVal() };
			idx++;
		}

		idx = 0;
		for (auto state : *automata.getStates()) {
			if (&state.data() == automata.getStartState()) {
				mStartState = mCurrentState = idx;
			}
			idx++;
		}

		ualni stateIdx = 0;
		for (auto state : *automata.getStates()) {
			for (auto transition : *state->getTransitions()) {
				ualni stateIdx2 = 0;
				for (auto state2 : *automata.getStates()) {
					if (transition->getState() == &state2.data()) break;
					stateIdx2++;
				}
				auto const code = transition->getSymbol();
				mTable.set({ (ualni) (code - mSymbolRange.mBegin), (ualni) stateIdx }, stateIdx2);
			}
			stateIdx++;
		}
	}
}