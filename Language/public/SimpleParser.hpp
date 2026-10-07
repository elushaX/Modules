
#pragma once

#include "Parser.hpp"
#include "UnifiedGrammarFormat.hpp"

namespace tp {

	// Gives ability to express grammar in the Unified Format as sentence
	template <typename tAlphabetType>
	class SimpleParser {
		using Def = UnifiedGrammarDefinition<tAlphabetType>;
		using Translator = UnifiedGrammarTranslator<tAlphabetType>;

		using UGParser = typename Def::GrammarParser;
		using UGRegexTokensParser = typename Def::RegexParser;
		using UserParser = Parser<tAlphabetType, alni, -1, 0, 127>;

	public:
		SimpleParser() {
			// Grammar for unified grammar format sentence that tables compiled from

			// Define Context-Free grammar
			ContextFreeGrammar contextFreeGrammar;
			{
				// use existing CF grammar interface
				Def::buildGrammarStructure(contextFreeGrammar, mUnifiedTerminals);
				contextFreeGrammar.setStart("Grammar");
			}

			// Define Regular grammar
			RegularGrammar<tAlphabetType, typename Def::GTokens> regularGrammar;
			{
				// this is basically ast from existing tokenizer
				Def::buildGrammarTokenizer(regularGrammar);
			}

			mCompileOk = mUnifiedGrammarParser.compileTables(contextFreeGrammar, regularGrammar, mUnifiedTerminals);
			mUnifiedGrammarParser.setIgnoreToken(Def::GIgnore);

			// Regular grammar used to parse the inline regex syntax inside "..."
			{
				ContextFreeGrammar regexContextFreeGrammar;
				RegularGrammar<tAlphabetType, typename Def::RTokens> regexRegularGrammar;

				Def::buildRegexStructure(regexContextFreeGrammar, mRegexTerminals);
				regexContextFreeGrammar.setStart("Alt");
				Def::buildRegexTokenizer(regexRegularGrammar);

				mCompileOk = mCompileOk && mRegexParser.compileTables(regexContextFreeGrammar, regexRegularGrammar, mRegexTerminals);
				mRegexParser.setIgnoreToken(Def::RNone);
			}
		}

	public:
		bool compileTables(const tAlphabetType* grammar, ualni grammarLength) {
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
			Translator translator;
			translator.translate(
				parsed.ast, grammar,
				userContextFreeGrammar, userRegularGrammar, terminalsMap,
				mUnifiedGrammarParser, mRegexParser
			);

			// compile tables from user grammar
			bool ok = mUserParser.compileTables(userContextFreeGrammar, userRegularGrammar, terminalsMap);
			mUserParser.setIgnoreToken(0);
			return ok;
		}

		typename UserParser::ParseResult parse(const tAlphabetType* grammar, ualni grammarLength) {
			return mUserParser.parse(grammar, grammarLength);
		}

		[[nodiscard]] bool isCompiled() const { return mCompileOk; }

	private:
		UGParser mUnifiedGrammarParser;
		UGRegexTokensParser mRegexParser;

		Map<String, typename Def::GTokens> mUnifiedTerminals;
		Map<String, typename Def::RTokens> mRegexTerminals;

		bool mCompileOk = false;

		UserParser mUserParser;
	};
}
