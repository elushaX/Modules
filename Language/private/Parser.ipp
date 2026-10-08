namespace tp {
	template <typename tAlphabetType, typename tTokenType, tTokenType tInTransition, ualni MinSymbol, ualni MaxSymbol>
	void Parser<tAlphabetType, tTokenType, tInTransition, MinSymbol, MaxSymbol>::setIgnoreToken(tTokenType token) { mIgnoreToken = token; }

	template <typename tAlphabetType, typename tTokenType, tTokenType tInTransition, ualni MinSymbol, ualni MaxSymbol>
	bool Parser<tAlphabetType, tTokenType, tInTransition, MinSymbol, MaxSymbol>::compileTables(
		const ContextFreeGrammar& cfGrammar,
		const ParserRegularGrammar& reGrammar,
		const Map<String, tTokenType>& contextFreeToRegular
	) {
		// Compile Regular Grammar
		{
			RegularGraph graph;
			ParserRegularCompiler compiler;
			compiler.compile(graph, reGrammar);
			graph.makeDeterministic();
			mRegularAutomata.construct(graph);
		}

		// compile context free grammar
		{
			ContextFreeCompiler compiler;
			if (!compiler.compile(cfGrammar)) return false;
			mContextFreeAutomata.construct(compiler);

			// make glue
			for (auto symbol : *compiler.getSymbols()) {
				auto symbolId = compiler.getSymbolId(symbol->mId);
				if (symbol->mIsTerminal) {
					auto iter = contextFreeToRegular.presents(symbol->mId);
					if (!iter) {
						return false;
					}
					mGrammarGlue.put(contextFreeToRegular.getSlotVal(iter), symbolId);
				} else {
					mAstNames.put(symbolId, symbol->mId);
				}
			}
		}

		return true;
	}

	template <typename tAlphabetType, typename tTokenType, tTokenType tInTransition, ualni MinSymbol, ualni MaxSymbol>
	auto Parser<tAlphabetType, tTokenType, tInTransition, MinSymbol, MaxSymbol>::parse(const tAlphabetType* sentence, ualni sentenceLength) -> ParseResult {
		// get tokens stream
		Buffer<ContextFreeCompiler::SymbolVal> tokens;

		const tAlphabetType* sentenceIter = sentence;
		ualni lengthIter = sentenceLength;

		while (lengthIter) {
			auto result = mRegularAutomata.accept(sentenceIter, lengthIter);

			if (!result.accepted) {
				return { false, nullptr };
			}

			if (result.state != mIgnoreToken) {
				if (!mGrammarGlue.presents(result.state)) {
					return { false, nullptr };
				}
				tokens.append(ContextFreeCompiler::SymbolVal(
					mGrammarGlue.get(result.state), ualni(sentenceIter - sentence), result.advancedIdx
				));
			}

			sentenceIter += result.advancedIdx;
			lengthIter -= result.advancedIdx;
		}

		ParserContextFreeAutomata::AcceptResult result = mContextFreeAutomata.accept(tokens.getBuff(), tokens.size());
		return { result.accepted, result.ast };
	}

}