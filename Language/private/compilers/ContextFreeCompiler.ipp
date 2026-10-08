namespace tp {
	inline bool ContextFreeCompiler::compile(const ContextFreeGrammar& grammar) {
		if (!init(grammar)) return false;
		return buildTable(grammar);
	}

	inline const Buffer<ContextFreeCompiler::Symbol>* ContextFreeCompiler::getSymbols() const { return &mSymbols; }
	inline ContextFreeCompiler::SymbolVal ContextFreeCompiler::getSymbolId(const String& name) const { return mSymbolLookup.get(name); }
	inline const Buffer<ContextFreeCompiler::SymbolVal>* ContextFreeCompiler::getTerminals() const { return &mTerminalSymbols; }
	inline const Buffer<ContextFreeCompiler::SymbolVal>* ContextFreeCompiler::getNonTerminals() const { return &mNonTerminalSymbols; }
	inline ContextFreeCompiler::SymbolVal ContextFreeCompiler::getStartSymbol() const { return mStartSymbol; }

	inline const Buffer2D<ContextFreeCompiler::Action>* ContextFreeCompiler::getActionTable() const { return &mActionTable; }
	inline const Buffer2D<alni>* ContextFreeCompiler::getGotoTable() const { return &mGotoTable; }
	inline const Buffer<alni>* ContextFreeCompiler::getRuleLhs() const { return &mRuleLhs; }
	inline const Buffer<ualni>* ContextFreeCompiler::getRulePopCount() const { return &mRulePopCount; }

	inline bool ContextFreeCompiler::init(const ContextFreeGrammar& grammar) {

		if (!grammar.getRules()->size()) {
			return false;
		}

		for (auto rule : *grammar.getRules()) {
			if (!rule->getArgs()->size()) {
				return false;
			}
		}

		findNonTerminals(grammar);

		for (auto nonTerminal : mNonTerminals) {
			for (auto rule : nonTerminal->val.rules) {
				for (auto arg : *rule->getArgs()) {
					if (arg->isTerminal() || arg->isEpsilon()) continue;

					if (!mNonTerminals.presents(arg->getId())) {
						printf("Referenced non-terminal '%s' is not defined\n", arg->getId().read());
						return false;
					}
				}
			}
		}

		findAllReferences(grammar);

		for (auto nonTerminal : mNonTerminals) {
			if (!nonTerminal->val.references.size() && nonTerminal->key != grammar.getStartTerminal()) {
				printf("Non-terminal '%s' is defined but not used\n", nonTerminal->key.read());
				return false;
			}
		}

		Map<String, bool> productive;

		bool changed = true;
		while (changed) {
			changed = false;

			for (auto nonTerminal : mNonTerminals) {
				if (productive.presents(nonTerminal->key)) continue;

				for (auto rule : nonTerminal->val.rules) {
					bool allProductive = true;
					for (auto arg : *rule->getArgs()) {
						if (arg->isTerminal() || arg->isEpsilon()) continue;
						if (!productive.presents(arg->getId())) {
							allProductive = false;
							break;
						}
					}
					if (allProductive) {
						productive.put(nonTerminal->key, {});
						changed = true;
						break;
					}
				}
			}
		}

		for (auto nonTerminal : mNonTerminals) {
			if (!productive.presents(nonTerminal->key)) {
				printf("Non-terminal '%s' is not productive\n", nonTerminal->key.read());
				return false;
			}
		}

		return true;
	}

	inline void ContextFreeCompiler::findNonTerminals(const ContextFreeGrammar& grammar) {
		for (auto rule : *grammar.getRules()) {
			if (!mNonTerminals.presents(rule->getId())) {
				mNonTerminals.put(rule->getId(), {});
			}
			auto nonTerminal = &mNonTerminals.get(rule->getId());
			nonTerminal->rules.append(&rule.data());
		}
	}

	inline void ContextFreeCompiler::findAllReferences(const ContextFreeGrammar& grammar) {
		for (auto nonTerminal : mNonTerminals) {
			for (auto rule : nonTerminal->val.rules) {
				for (auto arg : *rule->getArgs()) {
					if (arg->isTerminal() || arg->isEpsilon()) continue;

					NonTerminal* reference = &mNonTerminals.get(arg->getId());
					nonTerminal->val.referencing.put(arg->getId(), reference);
					reference->references.put(nonTerminal->key, &nonTerminal->val);
				}
			}
		}
	}

	inline bool ContextFreeCompiler::argIsEpsilon(const ContextFreeGrammar::Arg& arg) { return arg.isEpsilon() || arg.getId().size() == 0; }

	inline bool ContextFreeCompiler::buildTable(const ContextFreeGrammar& grammar) {

		// ---- symbols ----
		mNonTerminalSymbols.clear();
		mTerminalSymbols.clear();
		mSymbols.clear();

		ualni symbolId = 0;
		for (auto nonTerminal : mNonTerminals) {
			mNonTerminalIdx.put(nonTerminal->key, mNonTerminalSymbols.size());
			mNonTerminalSymbols.append(SymbolVal(symbolId));
			mSymbols.append({ nonTerminal->key, false });
			mSymbolLookup.put(nonTerminal->key, SymbolVal(symbolId));
			symbolId++;
		}

		for (auto rule : *grammar.getRules()) {
			for (auto arg : *rule->getArgs()) {
				if (argIsEpsilon(arg.data())) continue;
				if (!arg.data().isTerminal()) continue;

				if (!mTerminalIdx.presents(arg.data().getId())) {
					mTerminalIdx.put(arg.data().getId(), mTerminalSymbols.size());
					mTerminalSymbols.append(SymbolVal(symbolId));
					mSymbols.append({ arg.data().getId(), true });
					mSymbolLookup.put(arg.data().getId(), SymbolVal(symbolId));
					symbolId++;
				}
			}
		}

		const ualni N = mNonTerminalSymbols.size();
		const ualni T = mTerminalSymbols.size();

		mStartSymbol = mNonTerminalSymbols[mNonTerminalIdx.get(grammar.getStartTerminal())];

		// global symbol space: non-terminal 0..N-1, terminal N..N+T-1, EOF N+T
		auto argSymbol = [&](const ContextFreeGrammar::Arg& arg) -> alni {
			if (argIsEpsilon(arg)) return -1;
			if (arg.isTerminal()) return (alni) (N + mTerminalIdx.get(arg.getId()));
			return (alni) mNonTerminalIdx.get(arg.getId());
		};

		// ---- rules (rule 0 = augmented S' -> Start) ----
		mRuleLhs.clear();
		mRulePopCount.clear();

		Buffer<Buffer<alni>> ruleRhs;

		// augmented
		mRuleLhs.append((alni) N);
		mRulePopCount.append(ualni(1));
		Buffer<alni> startRhs;
		startRhs.append((alni) mNonTerminalIdx.get(grammar.getStartTerminal()));
		ruleRhs.append(startRhs);

		// rules for each non-terminal
		Buffer<Buffer<ualni>> rulesForNonTerminal;
		for (ualni i = 0; i < N; i++) rulesForNonTerminal.append(Buffer<ualni>());

		for (auto rule : *grammar.getRules()) {
			ualni lhs = mNonTerminalIdx.get(rule->getId());
			mRuleLhs.append((alni) lhs);
			Buffer<alni> rhs;
			for (auto arg : *rule->getArgs()) {
				alni sym = argSymbol(arg.data());
				if (sym < 0) continue;
				rhs.append(sym);
			}
			mRulePopCount.append(rhs.size());
			ruleRhs.append(rhs);
			rulesForNonTerminal[lhs].append(ruleRhs.size() - 1);
		}

		const ualni R = ruleRhs.size();

		// ---- FIRST + nullable ----
		Buffer<bool> nullable;
		nullable.reserve(N + T);
		for (ualni i = 0; i < N + T; i++) nullable.append(false);

		Buffer2D<bool> first;
		first.reserve({ N + T, T });

		for (ualni t = 0; t < T; t++) {
			first.set({ N + t, t }, true);
		}

		bool changed = true;
		while (changed) {
			changed = false;

			for (ualni r = 1; r < R; r++) {
				ualni lhs = (ualni) mRuleLhs[r];

				bool allNullable = true;
				for (auto sym : ruleRhs[r]) {
					if (!nullable[sym.data()]) allNullable = false;
				}

				if (allNullable && !nullable[lhs]) {
					nullable[lhs] = true;
					changed = true;
				}

				for (auto sym : ruleRhs[r]) {
					for (ualni t = 0; t < T; t++) {
						if (first.get({ sym.data(), t }) && !first.get({ lhs, t })) {
							first.set({ lhs, t }, true);
							changed = true;
						}
					}
					if (!nullable[sym.data()]) break;
				}
			}
		}

		// ---- LR(1) item sets ----
		auto addItem = [&](Buffer<LrItem>& set, const LrItem& item) {
			for (auto it : set) {
				if (it.data().rule == item.rule && it.data().dot == item.dot && it.data().look == item.look) return;
			}
			set.append(item);
		};

		auto closure = [&](Buffer<LrItem>& set) {
			bool c = true;
			while (c) {
				c = false;
				ualni size = set.size();
				for (ualni i = 0; i < size; i++) {
					LrItem item = set[i];
					if (item.dot >= ruleRhs[item.rule].size()) continue;
					alni sym = ruleRhs[item.rule][item.dot];
					if (sym >= (alni) N) continue; // terminal

					ualni B = (ualni) sym;

					// FIRST(beta look)
					Buffer<bool> firstBeta;
					firstBeta.reserve(T);
					for (ualni t = 0; t < T; t++) firstBeta.append(false);

					bool betaNullable = true;
					for (ualni j = item.dot + 1; j < ruleRhs[item.rule].size(); j++) {
						alni sj = ruleRhs[item.rule][j];
						if (sj >= (alni) N) {
							firstBeta[sj - N] = true;
							betaNullable = false;
							break;
						}
						for (ualni t = 0; t < T; t++) {
							if (first.get({ (ualni) sj, t })) firstBeta[t] = true;
						}
						if (!nullable[sj]) {
							betaNullable = false;
							break;
						}
					}

					for (auto ruleIdx : rulesForNonTerminal[B]) {
						ualni br = ruleIdx.data();
						for (ualni t = 0; t < T; t++) {
							if (!firstBeta[t]) continue;
							LrItem ni;
							ni.rule = br;
							ni.dot = 0;
							ni.look = t;
							addItem(set, ni);
						}
						if (betaNullable) {
							LrItem ni;
							ni.rule = br;
							ni.dot = 0;
							ni.look = item.look;
							addItem(set, ni);
						}
					}
				}
				if (set.size() != size) c = true;
			}
		};

		auto gotoSet = [&](Buffer<LrItem>& set, alni symbol) -> Buffer<LrItem> {
			Buffer<LrItem> out;
			for (auto item : set) {
				if (item.data().dot >= ruleRhs[item.data().rule].size()) continue;
				if (ruleRhs[item.data().rule][item.data().dot] != symbol) continue;
				LrItem ni;
				ni.rule = item.data().rule;
				ni.dot = item.data().dot + 1;
				ni.look = item.data().look;
				addItem(out, ni);
			}
			if (!out.size()) return out;
			closure(out);
			return out;
		};

		auto setsEqual = [&](Buffer<LrItem>& a, Buffer<LrItem>& b) {
			if (a.size() != b.size()) return false;
			for (auto item : a) {
				bool found = false;
				for (auto other : b) {
					if (other.data().rule == item.data().rule && other.data().dot == item.data().dot && other.data().look == item.data().look) {
						found = true;
						break;
					}
				}
				if (!found) return false;
			}
			return true;
		};

		// ---- canonical LR(1) collection ----
		Buffer<Buffer<LrItem>> states;
		Buffer<Edge> edges;

		Buffer<LrItem> startState;
		LrItem startItem;
		startItem.rule = 0;
		startItem.dot = 0;
		startItem.look = T;
		addItem(startState, startItem);
		closure(startState);
		states.append(startState);

		for (ualni s = 0; s < states.size(); s++) {
			for (alni symbol = 0; symbol < (alni) (N + T); symbol++) {
				Buffer<LrItem> g = gotoSet(states[s], symbol);
				if (!g.size()) continue;

				alni found = -1;
				for (ualni k = 0; k < states.size(); k++) {
					if (setsEqual(states[k], g)) {
						found = (alni) k;
						break;
					}
				}

				ualni target;
				if (found >= 0) {
					target = (ualni) found;
				} else {
					target = states.size();
					states.append(g);
				}

				edges.append({ s, symbol, target });
			}
		}

		// ---- terminal precedence (level + associativity) ----
		Buffer<ualni> termPrec;
		Buffer<alni> termAssoc;
		termPrec.reserve(T);
		termAssoc.reserve(T);
		for (ualni t = 0; t < T; t++) {
			termPrec.append(ualni(0));
			termAssoc.append(alni(-1));
		}

		for (auto p : *grammar.getTerminalPrecedence()) {
			auto iter = mTerminalIdx.presents(p->key);
			if (iter) {
				termPrec[mTerminalIdx.get(p->key)] = p->val;
			}
		}
		for (auto p : *grammar.getTerminalAssoc()) {
			auto iter = mTerminalIdx.presents(p->key);
			if (iter) {
				termAssoc[mTerminalIdx.get(p->key)] = p->val;
			}
		}

		// ---- rule precedence (rightmost terminal) ----
		Buffer<ualni> rulePrec;
		Buffer<alni> ruleAssoc;
		rulePrec.reserve(R);
		ruleAssoc.reserve(R);
		for (ualni r = 0; r < R; r++) {
			rulePrec.append(ualni(0));
			ruleAssoc.append(alni(-1));
			for (alni i = (alni) ruleRhs[r].size() - 1; i >= 0; i--) {
				alni sym = ruleRhs[r][i];
				if (sym >= (alni) N) {
					ualni t = (ualni) (sym - N);
					rulePrec[r] = termPrec[t];
					ruleAssoc[r] = termAssoc[t];
					break;
				}
			}
		}

		// ---- build action/goto tables ----
		const ualni S = states.size();

		mActionTable.reserve({ S, T + 1 });
		mActionTable.assign(Action{ Action::ERROR, 0 });

		mGotoTable.reserve({ S, N });
		mGotoTable.assign(-1);

		for (auto edge : edges) {
			if (edge.data().symbol >= (alni) N) {
				Action a;
				a.type = Action::SHIFT;
				a.num = edge.data().to;
				mActionTable.set({ edge.data().from, (ualni) (edge.data().symbol - N) }, a);
			} else {
				mGotoTable.set({ edge.data().from, (ualni) edge.data().symbol }, (alni) edge.data().to);
			}
		}

		for (ualni s = 0; s < S; s++) {
			for (auto item : states[s]) {
				if (item.data().dot != ruleRhs[item.data().rule].size()) continue;

				if (item.data().rule == 0) {
					Action a;
					a.type = Action::ACCEPT;
					a.num = 0;
					mActionTable.set({ s, T }, a);
				} else {
					Action a;
					a.type = Action::REDUCE;
					a.num = item.data().rule;
					Action& cell = mActionTable.get({ s, item.data().look });

					if (cell.type == Action::ERROR) {
						cell = a;
					} else if (cell.type == Action::SHIFT) {
						ualni shiftPrec = termPrec[item.data().look];
						alni shiftAssoc = termAssoc[item.data().look];
						ualni redPrec = rulePrec[a.num];
						alni redAssoc = ruleAssoc[a.num];

						if (shiftPrec == 0 || redPrec == 0 || (shiftPrec == redPrec && shiftAssoc == -1)) {
							printf("Grammar is not LALR(1) (unresolved shift/reduce conflict)\n");
							return false;
						}

						if (redPrec > shiftPrec || (redPrec == shiftPrec && shiftAssoc == 1)) {
							cell = a;
						}
					} else if (cell.type == Action::REDUCE && cell.num != a.num) {
						ualni p1 = rulePrec[cell.num];
						ualni p2 = rulePrec[a.num];
						if (p1 == 0 || p2 == 0 || p1 == p2) {
							printf("Grammar is not LALR(1) (unresolved reduce/reduce conflict)\n");
							return false;
						}
						if (p2 > p1) {
							cell = a;
						}
					}
				}
			}
		}

		return true;
	}
}