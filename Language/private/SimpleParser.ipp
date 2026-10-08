
#include "bootstrap/Translator.hpp"

namespace tp {

template <typename tAlphabetType>
SimpleParser<tAlphabetType>::SimpleParser() {
	// Grammar for unified grammar format sentence that tables compiled from

	// Define Context-Free grammar
	ContextFreeGrammar contextFreeGrammar;
	{
		// use existing CF grammar interface
		CFGDefinition<tAlphabetType>::buildGrammarStructure(contextFreeGrammar, mUnifiedTerminals);
		contextFreeGrammar.setStart("Grammar");
	}

	// Define Regular grammar
	RegularGrammar<tAlphabetType, typename CFGDefinition<tAlphabetType>::GTokens> regularGrammar;
	{
		// this is basically ast from existing tokenizer
		CFGDefinition<tAlphabetType>::buildGrammarTokenizer(regularGrammar);
	}

	mCompileOk = mUnifiedGrammarParser.compileTables(contextFreeGrammar, regularGrammar, mUnifiedTerminals);
	mUnifiedGrammarParser.setIgnoreToken(CFGDefinition<tAlphabetType>::GIgnore);

	// Regular grammar used to parse the inline regex syntax inside "..."
	{
		ContextFreeGrammar regexContextFreeGrammar;
		RegularGrammar<tAlphabetType, typename RegexDefinition<tAlphabetType>::RTokens> regexRegularGrammar;

		RegexDefinition<tAlphabetType>::buildRegexStructure(regexContextFreeGrammar, mRegexTerminals);
		regexContextFreeGrammar.setStart("Alt");
		RegexDefinition<tAlphabetType>::buildRegexTokenizer(regexRegularGrammar);

		mCompileOk = mCompileOk && mRegexParser.compileTables(regexContextFreeGrammar, regexRegularGrammar, mRegexTerminals);
		mRegexParser.setIgnoreToken(RegexDefinition<tAlphabetType>::RNone);
	}
}

template <typename tAlphabetType>
bool SimpleParser<tAlphabetType>::compileTables(const tAlphabetType* grammar, ualni grammarLength) {
	if (!mCompileOk) return false;

	auto parsed = mUnifiedGrammarParser.parse(grammar, grammarLength);
	if (!parsed.accepted || !parsed.ast) return false;

	// compile each ast into RegularGrammar and ContextFree Grammar api instructions
	ContextFreeGrammar userContextFreeGrammar;
	RegularGrammar<tAlphabetType, alni> userRegularGrammar;
	Map<String, alni> terminalsMap;

	// split ast into RE and CF part
	// generate and execute grammar api commands
	// use existing tokenizer code to create RE transition matrix
	mActions.clear();
	UnifiedGrammarTranslator<tAlphabetType> translator;
	translator.translate(
		parsed.ast, grammar,
		userContextFreeGrammar, userRegularGrammar, terminalsMap, mActions,
		mUnifiedGrammarParser, mRegexParser
	);

	// compile tables from user grammar
	bool ok = mUserParser.compileTables(userContextFreeGrammar, userRegularGrammar, terminalsMap);
	mUserParser.setIgnoreToken(0);
	return ok;
}

template <typename tAlphabetType>
auto SimpleParser<tAlphabetType>::parse(const tAlphabetType* grammar, ualni grammarLength) -> typename UserParser::ParseResult {
	return mUserParser.parse(grammar, grammarLength);
}

template <typename tAlphabetType>
bool SimpleParser<tAlphabetType>::isCompiled() const {
	return mCompileOk;
}

// ---- semantic-action accessors for walking the result AST ----

// action tag assigned to the production that reduced this node (empty if none / terminal)
template <typename tAlphabetType>
auto SimpleParser<tAlphabetType>::actionOf(const StackItem* node) const -> String {
	// compiler rule 0 is the augmented start; our actions are 0-based over the user productions
	if (!node || node->rule <= 0) return String("");
	ualni idx = (ualni) (node->rule - 1);
	if (idx >= mActions.size()) return String("");
	return mActions[idx];
}

// source text covered by a node's terminal symbol
template <typename tAlphabetType>
auto SimpleParser<tAlphabetType>::lexemeOf(const tAlphabetType* source, const StackItem* node) const -> String {
	char buf[512];
	ualni n = node->symbol.len < 511 ? node->symbol.len : 511;
	for (ualni i = 0; i < n; i++) buf[i] = (char) source[node->symbol.start + i];
	buf[n] = 0;
	return String(buf);
}

}
