#include "CLR.hpp"
#include "NewPlacement.hpp"

using namespace tp;

void CLR::setGrammar(const tp::CfGrammar& grammar) {
	if (grammar.isLooped()) {
		mState = ParserState::FAILED;
		mBuildError.description = "Cant build lalr parser from looped context free grammar";
		return;
	}

	mState = ParserState::FAILED;
}

void CLR::setTerminal(const String& name, TerminalStream::TerminalID id) {

}

void CLR::build() {

}

void CLR::parse(TerminalStream* source, List<ASTNode>& out, ASTNode** root) {
	*root = nullptr;
}