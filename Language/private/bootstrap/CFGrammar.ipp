namespace tp {
	template <typename tAlphabetType>
	auto CFGDefinition<tAlphabetType>::gLetter(GRegexGrammar& rg) -> const GRegexNode* {
		Buffer<IterRange<tAlphabetType>> r;
		r.append(IterRange<tAlphabetType>((tAlphabetType) 'a', (tAlphabetType) ('z' + 1)));
		r.append(IterRange<tAlphabetType>((tAlphabetType) 'A', (tAlphabetType) ('Z' + 1)));
		r.append(IterRange<tAlphabetType>((tAlphabetType) '_', (tAlphabetType) ('_' + 1)));
		return rg.ranges(r);
	}

	template <typename tAlphabetType>
	auto CFGDefinition<tAlphabetType>::gAlnum(GRegexGrammar& rg) -> const GRegexNode* {
		Buffer<IterRange<tAlphabetType>> r;
		r.append(IterRange<tAlphabetType>((tAlphabetType) 'a', (tAlphabetType) ('z' + 1)));
		r.append(IterRange<tAlphabetType>((tAlphabetType) 'A', (tAlphabetType) ('Z' + 1)));
		r.append(IterRange<tAlphabetType>((tAlphabetType) '0', (tAlphabetType) ('9' + 1)));
		r.append(IterRange<tAlphabetType>((tAlphabetType) '_', (tAlphabetType) ('_' + 1)));
		return rg.ranges(r);
	}

	template <typename tAlphabetType>
	auto CFGDefinition<tAlphabetType>::gNot(GRegexGrammar& rg, tAlphabetType q) -> const GRegexNode* {
		Buffer<IterRange<tAlphabetType>> r;
		r.append(IterRange<tAlphabetType>(q, (tAlphabetType) (q + 1)));
		return rg.ranges(r, true);
	}

	template <typename tAlphabetType>
	void CFGDefinition<tAlphabetType>::buildGrammarTokenizer(GRegexGrammar& rg) {
		rg.addRule(rg.seq({ gLetter(rg), rg.rep(gAlnum(rg)) }), GIdent);
		rg.addRule(rg.seq({ rg.val((tAlphabetType) '\''), rg.rep(gNot(rg, (tAlphabetType) '\'')), rg.val((tAlphabetType) '\'') }), GLiteral);
		// a regex literal "..." where \" is an escaped quote: " ( \<any> | not(") )* "
		rg.addRule(
			rg.seq({
				rg.val((tAlphabetType) '"'),
				rg.rep(rg.alt(rg.seq({ rg.val((tAlphabetType) '\\'), rg.any() }), gNot(rg, (tAlphabetType) '"'))),
				rg.val((tAlphabetType) '"')
			}),
			GRegex
		);
		rg.addRule(rg.seq({ rg.val((tAlphabetType) '['), rg.rep(gNot(rg, (tAlphabetType) ']')), rg.val((tAlphabetType) ']') }), GAction);
		rg.addRule(rg.val((tAlphabetType) '%'), GPercent);
		rg.addRule(rg.val((tAlphabetType) '{'), GLBrace);
		rg.addRule(rg.val((tAlphabetType) '}'), GRBrace);
		rg.addRule(rg.val((tAlphabetType) ':'), GColon);
		rg.addRule(rg.val((tAlphabetType) ';'), GSemi);
		rg.addRule(rg.val((tAlphabetType) '|'), GPipe);
		rg.addRule(
			rg.rep(
				rg.alt(
					rg.alt(
						rg.alt(rg.val((tAlphabetType) ' '), rg.val((tAlphabetType) '\t')),
						rg.val((tAlphabetType) '\n')
					),
					rg.val((tAlphabetType) '\r')
				),
				true
			),
			GIgnore
		);
	}

	template <typename tAlphabetType>
	void CFGDefinition<tAlphabetType>::buildGrammarStructure(ContextFreeGrammar& cfg, Map<String, GTokens>& tm) {
		tm.put("Ident", GIdent);
		tm.put("Literal", GLiteral);
		tm.put("Regex", GRegex);
		tm.put("Action", GAction);
		tm.put("Percent", GPercent);
		tm.put("LBrace", GLBrace);
		tm.put("RBrace", GRBrace);
		tm.put("Colon", GColon);
		tm.put("Semi", GSemi);
		tm.put("Pipe", GPipe);

		auto eps = [&](const char* name) {
			auto rule = cfg.addRule(String(name));
			rule->addArg(ContextFreeGrammar::Arg(String(""), false, true));
		};

		auto term = [&](const char* lhs, const char* t) {
			auto rule = cfg.addRule(String(lhs));
			rule->addArg(ContextFreeGrammar::Arg(String(t), true));
		};

		auto nt = [&](const char* lhs, const char* t) {
			auto rule = cfg.addRule(String(lhs));
			rule->addArg(ContextFreeGrammar::Arg(String(t), false));
		};

		// Grammar := Ident LBrace Body RBrace
		{
			auto rule = cfg.addRule(String("Grammar"));
			rule->addArg(ContextFreeGrammar::Arg(String("Ident"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("LBrace"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Body"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("RBrace"), true));
		}

		// Body := Item Body | eps
		{
			auto rule = cfg.addRule(String("Body"));
			rule->addArg(ContextFreeGrammar::Arg(String("Item"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Body"), false));
		}
		eps("Body");

		nt("Item", "Directive");
		nt("Item", "Production");

		// Directive := Percent Ident Args Semi
		{
			auto rule = cfg.addRule(String("Directive"));
			rule->addArg(ContextFreeGrammar::Arg(String("Percent"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Ident"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Args"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Semi"), true));
		}

		// Args := Arg Args | eps
		{
			auto rule = cfg.addRule(String("Args"));
			rule->addArg(ContextFreeGrammar::Arg(String("Arg"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Args"), false));
		}
		eps("Args");

		// Arg := Literal | Ident | Regex
		term("Arg", "Literal");
		term("Arg", "Ident");
		term("Arg", "Regex");

		// Production := Ident Colon Alts Semi
		{
			auto rule = cfg.addRule(String("Production"));
			rule->addArg(ContextFreeGrammar::Arg(String("Ident"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Colon"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Alts"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Semi"), true));
		}

		// Alts := Alt AltsRest
		{
			auto rule = cfg.addRule(String("Alts"));
			rule->addArg(ContextFreeGrammar::Arg(String("Alt"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("AltsRest"), false));
		}

		// AltsRest := Pipe Alt AltsRest | eps
		{
			auto rule = cfg.addRule(String("AltsRest"));
			rule->addArg(ContextFreeGrammar::Arg(String("Pipe"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Alt"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("AltsRest"), false));
		}
		eps("AltsRest");

		// Alt := Symbols OptAction
		{
			auto rule = cfg.addRule(String("Alt"));
			rule->addArg(ContextFreeGrammar::Arg(String("Symbols"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("OptAction"), false));
		}

		// Symbols := Symbol Symbols | eps
		{
			auto rule = cfg.addRule(String("Symbols"));
			rule->addArg(ContextFreeGrammar::Arg(String("Symbol"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Symbols"), false));
		}
		eps("Symbols");

		// Symbol := Literal | Regex | Ident
		term("Symbol", "Literal");
		term("Symbol", "Regex");
		term("Symbol", "Ident");

		// OptAction := Action | eps
		term("OptAction", "Action");
		eps("OptAction");
	}
}