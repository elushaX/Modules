
#include "UnifiedGrammarFormat.hpp"

using namespace tp;

// ============================= UnifiedGrammarDefinition =============================

template <typename tAlphabetType>
auto UnifiedGrammarDefinition<tAlphabetType>::gLetter(GRegexGrammar& rg) -> const GRegexNode* {
	Buffer<IterRange<tAlphabetType>> r;
	r.append(IterRange<tAlphabetType>((tAlphabetType) 'a', (tAlphabetType) ('z' + 1)));
	r.append(IterRange<tAlphabetType>((tAlphabetType) 'A', (tAlphabetType) ('Z' + 1)));
	r.append(IterRange<tAlphabetType>((tAlphabetType) '_', (tAlphabetType) ('_' + 1)));
	return rg.ranges(r);
}

template <typename tAlphabetType>
auto UnifiedGrammarDefinition<tAlphabetType>::gAlnum(GRegexGrammar& rg) -> const GRegexNode* {
	Buffer<IterRange<tAlphabetType>> r;
	r.append(IterRange<tAlphabetType>((tAlphabetType) 'a', (tAlphabetType) ('z' + 1)));
	r.append(IterRange<tAlphabetType>((tAlphabetType) 'A', (tAlphabetType) ('Z' + 1)));
	r.append(IterRange<tAlphabetType>((tAlphabetType) '0', (tAlphabetType) ('9' + 1)));
	r.append(IterRange<tAlphabetType>((tAlphabetType) '_', (tAlphabetType) ('_' + 1)));
	return rg.ranges(r);
}

template <typename tAlphabetType>
auto UnifiedGrammarDefinition<tAlphabetType>::gNot(GRegexGrammar& rg, tAlphabetType q) -> const GRegexNode* {
	Buffer<IterRange<tAlphabetType>> r;
	r.append(IterRange<tAlphabetType>(q, (tAlphabetType) (q + 1)));
	return rg.ranges(r, true);
}

template <typename tAlphabetType>
auto UnifiedGrammarDefinition<tAlphabetType>::rNot(RRegexGrammar& rg, tAlphabetType q) -> const RRegexNode* {
	Buffer<IterRange<tAlphabetType>> r;
	r.append(IterRange<tAlphabetType>(q, (tAlphabetType) (q + 1)));
	return rg.ranges(r, true);
}

template <typename tAlphabetType>
auto UnifiedGrammarDefinition<tAlphabetType>::rAnyChar(RRegexGrammar& rg) -> const RRegexNode* {
	Buffer<IterRange<tAlphabetType>> r;
	r.append(IterRange<tAlphabetType>((tAlphabetType) 1, (tAlphabetType) 127));
	return rg.ranges(r);
}

template <typename tAlphabetType>
void UnifiedGrammarDefinition<tAlphabetType>::buildGrammarTokenizer(GRegexGrammar& rg) {
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
void UnifiedGrammarDefinition<tAlphabetType>::buildGrammarStructure(ContextFreeGrammar& cfg, Map<String, GTokens>& tm) {
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

template <typename tAlphabetType>
void UnifiedGrammarDefinition<tAlphabetType>::buildRegexTokenizer(RRegexGrammar& rg) {
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
void UnifiedGrammarDefinition<tAlphabetType>::buildRegexStructure(ContextFreeGrammar& cfg, Map<String, RTokens>& tm) {
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

// ============================= UnifiedGrammarTranslator =============================

template <typename tAlphabetType>
bool UnifiedGrammarTranslator<tAlphabetType>::strEq(const char* a, const char* b) {
	while (*a && *b) {
		if (*a != *b) return false;
		a++;
		b++;
	}
	return *a == *b;
}

template <typename tAlphabetType>
String UnifiedGrammarTranslator<tAlphabetType>::lexeme(const tAlphabetType* text, const ContextFreeCompiler::SymbolVal& sym) {
	char buf[512];
	ualni n = sym.len < 511 ? sym.len : 511;
	for (ualni i = 0; i < n; i++) buf[i] = (char) text[sym.start + i];
	buf[n] = 0;
	return String(buf);
}

template <typename tAlphabetType>
char UnifiedGrammarTranslator<tAlphabetType>::decodeEscape(char e) {
	switch (e) {
		case 'b': return '\b';
		case 'f': return '\f';
		case 'n': return '\n';
		case 'r': return '\r';
		case 't': return '\t';
		default: return e;
	}
}

template <typename tAlphabetType>
String UnifiedGrammarTranslator<tAlphabetType>::stripQuotes(const String& text) {
	ualni len = text.size() - 2;
	char* buf = new char[len + 1];
	for (ualni i = 0; i < len; i++) buf[i] = text.read()[i + 1];
	buf[len] = 0;
	String out(buf);
	delete[] buf;
	return out;
}

template <typename tAlphabetType>
String UnifiedGrammarTranslator<tAlphabetType>::nodeName(AstNode node) {
	return mGrammarParser->mAstNames.get(node->symbol);
}

template <typename tAlphabetType>
String UnifiedGrammarTranslator<tAlphabetType>::regexNodeName(AstNode node) {
	return mRegexParser->mAstNames.get(node->symbol);
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::translate(
	AstNode root,
	const tAlphabetType* text,
	ContextFreeGrammar& cf,
	UserRegexGrammar& rg,
	Map<String, alni>& terminals,
	Buffer<String>& actions,
	GrammarParser& grammarParser,
	RegexParser& regexParser
) {
	mGrammarParser = &grammarParser;
	mRegexParser = &regexParser;

	BuildCtx ctx;
	ctx.text = text;
	ctx.cf = &cf;
	ctx.rg = &rg;
	ctx.terminals = &terminals;
	ctx.actions = &actions;
	ctx.nextToken = 1;

	walkGrammar(root, ctx);

	cf.setStart(ctx.start);
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::walkGrammar(AstNode node, BuildCtx& ctx) {
	walkBody(node->leafs[2], ctx);

	// whitespace owns token 0 (the ignore token), compiled before productions
	if (ctx.hasWhitespace) {
		const UserRegexNode* wsNode = compileRegexString(ctx.whitespace, ctx);
		if (wsNode) ctx.rg->addRule(wsNode, 0);
	}

	for (auto prod : ctx.productionNodes) {
		buildProduction(prod.data(), ctx);
	}
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::walkBody(AstNode node, BuildCtx& ctx) {
	if (node->leafs.size() == 0) return;
	walkItem(node->leafs[0], ctx);
	walkBody(node->leafs[1], ctx);
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::walkItem(AstNode node, BuildCtx& ctx) {
	AstNode child = node->leafs[0];
	String name = nodeName(child);

	if (strEq(name.read(), "Directive")) {
		walkDirective(child, ctx);
	} else {
		collectProduction(child, ctx);
	}
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::walkDirective(AstNode node, BuildCtx& ctx) {
	String word = lexeme(ctx.text, node->leafs[1]->symbol);
	AstNode args = node->leafs[2];

	Buffer<String> argTexts;
	collectArgTexts(args, ctx, argTexts);

	if (strEq(word.read(), "start")) {
		if (argTexts.size() > 0) {
			ctx.start = argTexts[0];
			ctx.hasStart = true;
		}
		return;
	}
	if (strEq(word.read(), "whitespace")) {
		if (argTexts.size() > 0) {
			ctx.whitespace = argTexts[0];
			ctx.hasWhitespace = true;
		}
		return;
	}

	alni assoc = -1;
	if (strEq(word.read(), "left")) assoc = 0;
	else if (strEq(word.read(), "right")) assoc = 1;

	for (auto sym : argTexts) {
		ctx.cf->setTerminalPrecedence(sym.data(), ctx.nextPrecedence, assoc);
	}
	ctx.nextPrecedence++;
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::collectArgTexts(AstNode args, BuildCtx& ctx, Buffer<String>& out) {
	if (args->leafs.size() == 0) return;
	AstNode arg = args->leafs[0];
	String text = lexeme(ctx.text, arg->leafs[0]->symbol);

	if (text.read()[0] == '\'' || text.read()[0] == '"') {
		out.append(stripQuotes(text));
	} else {
		out.append(text);
	}

	collectArgTexts(args->leafs[1], ctx, out);
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::collectProduction(AstNode node, BuildCtx& ctx) {
	String lhs = lexeme(ctx.text, node->leafs[0]->symbol);
	if (!ctx.hasStart) {
		ctx.start = lhs;
		ctx.hasStart = true;
	}

	AstNode alts = node->leafs[2];
	Buffer<AstNode> alternatives;
	collectAlts(alts, alternatives);

	// named terminal: single regex symbol, no action, single alternative
	if (alternatives.size() == 1) {
		AstNode altNode = alternatives[0];
		AstNode symbols = altNode->leafs[0];
		AstNode optAction = altNode->leafs[1];

		Buffer<AstNode> symbolNodes;
		collectSymbolNodes(symbols, symbolNodes);

		if (symbolNodes.size() == 1 && isRegexSymbol(symbolNodes[0], ctx) && optAction->leafs.size() == 0) {
			ctx.namedTerminals.put(lhs, true);
		}
	}

	ctx.productionNodes.append(node);
}

template <typename tAlphabetType>
bool UnifiedGrammarTranslator<tAlphabetType>::isRegexSymbol(AstNode sym, BuildCtx& ctx) {
	AstNode leaf = sym->leafs[0];
	String text = lexeme(ctx.text, leaf->symbol);
	return text.read()[0] == '"';
}

template <typename tAlphabetType>
String UnifiedGrammarTranslator<tAlphabetType>::altAction(AstNode altNode, BuildCtx& ctx) {
	AstNode optAction = altNode->leafs[1];
	if (optAction->leafs.size() == 0) return String("");
	// OptAction := Action ('[name]')
	String text = lexeme(ctx.text, optAction->leafs[0]->symbol);
	return stripQuotes(text); // strips the surrounding [ ]
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::buildProduction(AstNode node, BuildCtx& ctx) {
	String lhs = lexeme(ctx.text, node->leafs[0]->symbol);
	AstNode alts = node->leafs[2];

	Buffer<AstNode> alternatives;
	collectAlts(alts, alternatives);

	for (auto alt : alternatives) {
		AstNode altNode = alt.data();
		AstNode symbols = altNode->leafs[0];
		AstNode optAction = altNode->leafs[1];

		Buffer<AstNode> symbolNodes;
		collectSymbolNodes(symbols, symbolNodes);

		// named terminal
		if (symbolNodes.size() == 1 && isRegexSymbol(symbolNodes[0], ctx) && optAction->leafs.size() == 0 && alternatives.size() == 1) {
			String regexText = lexeme(ctx.text, symbolNodes[0]->leafs[0]->symbol);
			String inner = stripQuotes(regexText);
			const UserRegexNode* rgNode = compileRegexString(inner, ctx);
			if (rgNode) {
				ctx.rg->addRule(rgNode, ctx.nextToken);
				ctx.terminals->put(lhs, ctx.nextToken);
				ctx.nextToken++;
			}
			continue;
		}

		// production
		auto rule = ctx.cf->addRule(lhs);
		if (ctx.actions) ctx.actions->append(altAction(altNode, ctx));

		// empty alternative -> epsilon production
		if (symbolNodes.size() == 0) {
			rule->addArg(ContextFreeGrammar::Arg(String(""), false, true));
			continue;
		}

		for (auto sym : symbolNodes) {
			AstNode leaf = sym.data()->leafs[0];
			String text = lexeme(ctx.text, leaf->symbol);

			if (text.read()[0] == '\'') {
				String lit = stripQuotes(text);
				if (!ctx.terminals->presents(lit)) {
					ctx.terminals->put(lit, ctx.nextToken);
					ctx.rg->addRule(charSeq(lit, ctx), ctx.nextToken);
					ctx.nextToken++;
				}
				rule->addArg(ContextFreeGrammar::Arg(lit, true));
			} else if (text.read()[0] == '"') {
				String inner = stripQuotes(text);
				const UserRegexNode* rgNode = compileRegexString(inner, ctx);
				if (rgNode) {
					ctx.rg->addRule(rgNode, ctx.nextToken);
					ctx.terminals->put(inner, ctx.nextToken);
					ctx.nextToken++;
				}
				rule->addArg(ContextFreeGrammar::Arg(inner, true));
			} else if (ctx.namedTerminals.presents(text)) {
				rule->addArg(ContextFreeGrammar::Arg(text, true));
			} else {
				rule->addArg(ContextFreeGrammar::Arg(text, false));
			}
		}
	}
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::collectAlts(AstNode alts, Buffer<AstNode>& out) {
	out.append(alts->leafs[0]);
	collectAltsRest(alts->leafs[1], out);
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::collectAltsRest(AstNode rest, Buffer<AstNode>& out) {
	if (rest->leafs.size() == 0) return;
	out.append(rest->leafs[1]);
	collectAltsRest(rest->leafs[2], out);
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::collectSymbolNodes(AstNode symbols, Buffer<AstNode>& out) {
	if (symbols->leafs.size() == 0) return;
	out.append(symbols->leafs[0]);
	collectSymbolNodes(symbols->leafs[1], out);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileRegexString(const String& regex, BuildCtx& ctx) -> const UserRegexNode* {
	mRegexText = regex;
	auto parsed = mRegexParser->parse(regex.read(), regex.size());
	if (!parsed.accepted || !parsed.ast) {
		return nullptr;
	}
	return compileAlt(parsed.ast, ctx);
}

template <typename tAlphabetType>
String UnifiedGrammarTranslator<tAlphabetType>::rLexeme(const ContextFreeCompiler::SymbolVal& sym) {
	char buf[256];
	ualni n = sym.len < 255 ? sym.len : 255;
	for (ualni i = 0; i < n; i++) buf[i] = (char) mRegexText.read()[sym.start + i];
	buf[n] = 0;
	return String(buf);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileAlt(AstNode alt, BuildCtx& ctx) -> const UserRegexNode* {
	const UserRegexNode* node = compileConcat(alt->leafs[0], ctx);
	return compileAltRest(alt->leafs[1], node, ctx);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileAltRest(AstNode rest, const UserRegexNode* left, BuildCtx& ctx) -> const UserRegexNode* {
	if (rest->leafs.size() == 0) return left;
	const UserRegexNode* right = compileConcat(rest->leafs[1], ctx);
	left = ctx.rg->alt(left, right);
	return compileAltRest(rest->leafs[2], left, ctx);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileConcat(AstNode concat, BuildCtx& ctx) -> const UserRegexNode* {
	const UserRegexNode* node = compileRepeat(concat->leafs[0], ctx);
	return compileConcatRest(concat->leafs[1], node, ctx);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileConcatRest(AstNode rest, const UserRegexNode* left, BuildCtx& ctx) -> const UserRegexNode* {
	if (rest->leafs.size() == 0) return left;
	const UserRegexNode* right = compileRepeat(rest->leafs[0], ctx);
	left = ctx.rg->seq({ left, right });
	return compileConcatRest(rest->leafs[1], left, ctx);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileRepeat(AstNode repeat, BuildCtx& ctx) -> const UserRegexNode* {
	const UserRegexNode* node = compileAtom(repeat->leafs[0], ctx);
	AstNode suffix = repeat->leafs[1];
	if (suffix->leafs.size() == 0) return node;

	String s = rLexeme(suffix->leafs[0]->symbol);
	if (s.read()[0] == '*') return ctx.rg->rep(node);
	if (s.read()[0] == '+') return ctx.rg->rep(node, true);
	if (s.read()[0] == '?') return ctx.rg->may(node);
	return node;
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileAtom(AstNode atom, BuildCtx& ctx) -> const UserRegexNode* {
	AstNode child = atom->leafs[0];

	if (child->leafs.size() > 0) {
		String name = regexNodeName(child);
		if (strEq(name.read(), "Group")) {
			return compileAlt(child->leafs[1], ctx);
		}
		if (strEq(name.read(), "Class")) {
			return compileClass(child, ctx);
		}
	}

	// terminal leaf (Char, Escape, String, Minus, Caret)
	String text = rLexeme(child->symbol);

	if (text.read()[0] == '\\') {
		return ctx.rg->val((tAlphabetType) decodeEscape(text.read()[1]));
	}
	if (text.read()[0] == '"') {
		String inner = stripQuotes(text);
		return charSeq(inner, ctx);
	}
	return charSeq(text, ctx);
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::charSeq(const String& text, BuildCtx& ctx) -> const UserRegexNode* {
	const UserRegexNode* node = nullptr;
	for (const char* p = text.read(); *p; p++) {
		const UserRegexNode* ch = ctx.rg->val((tAlphabetType) *p);
		node = node ? ctx.rg->seq({ node, ch }) : ch;
	}
	if (!node) node = ctx.rg->val((tAlphabetType) 0);
	return node;
}

template <typename tAlphabetType>
auto UnifiedGrammarTranslator<tAlphabetType>::compileClass(AstNode cls, BuildCtx& ctx) -> const UserRegexNode* {
	bool exclude = cls->leafs[1]->leafs.size() > 0; // CaretOpt
	AstNode items = cls->leafs[2];

	Buffer<IterRange<tAlphabetType>> ranges;
	collectClassItems(items, ranges, ctx);

	return ctx.rg->ranges(ranges, exclude);
}

template <typename tAlphabetType>
char UnifiedGrammarTranslator<tAlphabetType>::classCharValue(AstNode classChar, BuildCtx& ctx) {
	String s = rLexeme(classChar->leafs[0]->symbol);
	if (s.read()[0] == '\\') return decodeEscape(s.read()[1]);
	return s.read()[0];
}

template <typename tAlphabetType>
void UnifiedGrammarTranslator<tAlphabetType>::collectClassItems(AstNode items, Buffer<IterRange<tAlphabetType>>& out, BuildCtx& ctx) {
	if (items->leafs.size() == 0) return;

	AstNode item = items->leafs[0];
	if (item->leafs.size() == 3) {
		char lo = classCharValue(item->leafs[0], ctx);
		char hi = classCharValue(item->leafs[2], ctx);
		out.append(IterRange<tAlphabetType>((tAlphabetType) lo, (tAlphabetType) (hi + 1)));
	} else {
		char c = classCharValue(item->leafs[0], ctx);
		out.append(IterRange<tAlphabetType>((tAlphabetType) c, (tAlphabetType) (c + 1)));
	}

	collectClassItems(items->leafs[1], out, ctx);
}

// ============================= explicit instantiation =============================

template struct tp::UnifiedGrammarDefinition<int1>;
template struct tp::UnifiedGrammarTranslator<int1>;
