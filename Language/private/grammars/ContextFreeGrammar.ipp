namespace tp {
	inline ContextFreeGrammar::Arg::Arg(const String& id, bool terminal, bool epsilon) {
		mId = id;
		mIsTerminal = terminal;
		mIsEpsilon = epsilon;
	}

	inline const String& ContextFreeGrammar::Arg::getId() const { return mId; }

	inline bool ContextFreeGrammar::Arg::operator==(const Arg& in) const {
		return (mId == in.mId) && (mIsEpsilon == in.mIsEpsilon) && (mIsTerminal == in.mIsTerminal);
	}

	inline ContextFreeGrammar::Rule::Rule(const String& id, const InitialierList<Arg>& args) {
		mId = id;
		mArgs = args;
	}

	inline bool ContextFreeGrammar::Rule::operator==(const Rule& in) const { return (mId == in.mId) && (mArgs == in.mArgs); }

	inline bool ContextFreeGrammar::Rule::isProductive() const {
		for (auto arg : mArgs) {
			if (arg->getId() == mId) return false;
		}
		return true;
	}

	inline void ContextFreeGrammar::addRule(const Rule& rule) { mRules.append(rule); }

	inline void ContextFreeGrammar::addRule(const String& id, const InitialierList<Arg>& args) { addRule(Rule(id, args)); }

	inline void ContextFreeGrammar::setStart(const String& startRule) { mStartTerminal = startRule; }
}