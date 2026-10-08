
namespace tp {
	template <typename tAlphabetType>
	auto RegexDefinition<tAlphabetType>::rNot(RRegexGrammar& rg, tAlphabetType q) -> const RRegexNode* {
		Buffer<IterRange<tAlphabetType>> r;
		r.append(IterRange<tAlphabetType>(q, (tAlphabetType) (q + 1)));
		return rg.ranges(r, true);
	}

	template <typename tAlphabetType>
	auto RegexDefinition<tAlphabetType>::rAnyChar(RRegexGrammar& rg) -> const RRegexNode* {
		Buffer<IterRange<tAlphabetType>> r;
		r.append(IterRange<tAlphabetType>((tAlphabetType) 1, (tAlphabetType) 127));
		return rg.ranges(r);
	}

	template <typename tAlphabetType>
	void RegexDefinition<tAlphabetType>::buildRegexTokenizer(RRegexGrammar& rg) {
		// Order matters: multi-char rules win via longest match.
		rg.addRule(rg.seq({ rg.val((tAlphabetType) '\\'), rg.any() }), REscape);
		rg.addRule(rg.seq({ rg.val((tAlphabetType) '"'), rg.rep(rNot(rg, (tAlphabetType) '"')), rg.val((tAlphabetType) '"') }), RString);
		rg.addRule(rg.val((tAlphabetType) '|'), RPipe);
		rg.addRule(rg.val((tAlphabetType) '*'), RStar);
		rg.addRule(rg.val((tAlphabetType) '+'), RPlus);
		rg.addRule(rg.val((tAlphabetType) '?'), RQuestion);
		rg.addRule(rg.val((tAlphabetType) '('), RLParen);
		rg.addRule(rg.val((tAlphabetType) ')'), RRParen);
		rg.addRule(rg.val((tAlphabetType) '['), RLBracket);
		rg.addRule(rg.val((tAlphabetType) ']'), RRBracket);
		rg.addRule(rg.val((tAlphabetType) '-'), RMinus);
		rg.addRule(rg.val((tAlphabetType) '^'), RCaret);
		rg.addRule(rAnyChar(rg), RChar);
	}

	template <typename tAlphabetType>
	void RegexDefinition<tAlphabetType>::buildRegexStructure(ContextFreeGrammar& cfg, Map<String, RTokens>& tm) {
		tm.put("Char", RChar);
		tm.put("Escape", REscape);
		tm.put("String", RString);
		tm.put("Pipe", RPipe);
		tm.put("Star", RStar);
		tm.put("Plus", RPlus);
		tm.put("Question", RQuestion);
		tm.put("LParen", RLParen);
		tm.put("RParen", RRParen);
		tm.put("LBracket", RLBracket);
		tm.put("RBracket", RRBracket);
		tm.put("Minus", RMinus);
		tm.put("Caret", RCaret);

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

		// Alt := Concat AltRest
		{
			auto rule = cfg.addRule(String("Alt"));
			rule->addArg(ContextFreeGrammar::Arg(String("Concat"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("AltRest"), false));
		}

		// AltRest := Pipe Concat AltRest | eps
		{
			auto rule = cfg.addRule(String("AltRest"));
			rule->addArg(ContextFreeGrammar::Arg(String("Pipe"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Concat"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("AltRest"), false));
		}
		eps("AltRest");

		// Concat := Repeat ConcatRest
		{
			auto rule = cfg.addRule(String("Concat"));
			rule->addArg(ContextFreeGrammar::Arg(String("Repeat"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("ConcatRest"), false));
		}

		// ConcatRest := Repeat ConcatRest | eps
		{
			auto rule = cfg.addRule(String("ConcatRest"));
			rule->addArg(ContextFreeGrammar::Arg(String("Repeat"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("ConcatRest"), false));
		}
		eps("ConcatRest");

		// Repeat := Atom Suffix
		{
			auto rule = cfg.addRule(String("Repeat"));
			rule->addArg(ContextFreeGrammar::Arg(String("Atom"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Suffix"), false));
		}

		// Suffix := Star | Plus | Question | eps
		term("Suffix", "Star");
		term("Suffix", "Plus");
		term("Suffix", "Question");
		eps("Suffix");

		// Atom := Char | Escape | String | Minus | Caret | Group | Class
		term("Atom", "Char");
		term("Atom", "Escape");
		term("Atom", "String");
		term("Atom", "Minus");
		term("Atom", "Caret");
		nt("Atom", "Group");
		nt("Atom", "Class");

		// Group := LParen Alt RParen
		{
			auto rule = cfg.addRule(String("Group"));
			rule->addArg(ContextFreeGrammar::Arg(String("LParen"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("Alt"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("RParen"), true));
		}

		// Class := LBracket CaretOpt Items RBracket
		{
			auto rule = cfg.addRule(String("Class"));
			rule->addArg(ContextFreeGrammar::Arg(String("LBracket"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("CaretOpt"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Items"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("RBracket"), true));
		}

		term("CaretOpt", "Caret");
		eps("CaretOpt");

		// Items := Item Items | eps
		{
			auto rule = cfg.addRule(String("Items"));
			rule->addArg(ContextFreeGrammar::Arg(String("Item"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Items"), false));
		}
		eps("Items");

		// Item := ClassChar Minus ClassChar | ClassChar
		{
			auto rule = cfg.addRule(String("Item"));
			rule->addArg(ContextFreeGrammar::Arg(String("ClassChar"), false));
			rule->addArg(ContextFreeGrammar::Arg(String("Minus"), true));
			rule->addArg(ContextFreeGrammar::Arg(String("ClassChar"), false));
		}
		{
			auto rule = cfg.addRule(String("Item"));
			rule->addArg(ContextFreeGrammar::Arg(String("ClassChar"), false));
		}

		// ClassChar := Char | Escape
		term("ClassChar", "Char");
		term("ClassChar", "Escape");
	}
}