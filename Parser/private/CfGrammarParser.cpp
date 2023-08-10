
#include "NewPlacement.hpp"

#include "CfGrammar.hpp"
#include "Tokenizer.hpp"

using namespace tp;

enum class Token {
	NO_TOKEN,
	START,
	ID,
	EQUAL,
	TERMINAL_START,
	TERMINAL_END,
	ALTERNATION,
	RULE_END,
	IGNORED,
	END,
	FAILED,
};

typedef int1 AlphabetType;
typedef SimpleTokenizer<AlphabetType, Token, Token::NO_TOKEN, Token::FAILED, Token::END> CFGTokenizer;

struct State {

	struct Transition {
		Token tok = Token::FAILED;
		State* target = nullptr;
		void (*action)(CfGrammar* grammar, const String& tok) = nullptr;
	};

	String error;
	Buffer<Transition> transitions;
	bool accept = false;
};

typedef Buffer<State> Automata;

void initializeTokenizer(CFGTokenizer* tok) {
	typedef decltype(tok->getTokenizer()) TokBase;
	tok->build({
			{ TokBase::idRE, Token::ID },
			{ TokBase::etherRE, Token::IGNORED },
			{ "Start", Token::START },
			{ ":", Token::EQUAL },
			{ "[", Token::TERMINAL_START },
			{ "]", Token::TERMINAL_END },
			{ "|", Token::ALTERNATION },
			{ ";", Token::RULE_END },
	});
}

void initAutomata(Automata& automata) {
	automata.reserve(8);
	auto states = &automata.first();

	auto root = states++;
	auto end = states++;
	auto start = states++;
	auto startEnd = states++;
	auto rule = states++;
	auto rhs = states++;
	auto terminalStart = states++;
	auto terminalEnd = states;

	root->transitions = {
			{ Token::END, end },
			{ Token::START, start },
			{ Token::ID, rule, [](CfGrammar* grammar, const String& tok) {
				 grammar->rules.pushBack({ tok, {} });
			 }
			},
	};

	start->transitions = {
			{ Token::ID, startEnd},
	};

	startEnd->transitions = {
			{ Token::RULE_END,
			 root,
			 [](CfGrammar* grammar, const String& tok) {
				 grammar->startTerminal = tok;
			 }
			},
	};

	rule->transitions = {
			{ Token::EQUAL, rhs },
	};

	rhs->transitions = {
			{ Token::ALTERNATION, rhs, [](CfGrammar* grammar, const String& tok) {
				 auto const& id = grammar->rules.last()->data.id;
				 grammar->rules.pushBack({ id, {} });
			 }
			},
			{ Token::ID, rhs, [](CfGrammar* grammar, const String& tok) {
				 grammar->rules.last()->data.args.pushBack({ tok, false });
			 }
			},
			{ Token::TERMINAL_START, terminalStart },
			{ Token::RULE_END, root },
	};

	terminalStart->transitions = {
			{ Token::ID, rhs, [](CfGrammar* grammar, const String& tok) {
				 grammar->rules.last()->data.args.pushBack({ tok, true });
			 }
			},
	};

	terminalEnd->transitions = {
			{ Token::TERMINAL_END, rhs },
	};

	auto idError = "Expected an identifier";
	auto stmEndError = "Expected an statement end ';'";

	root->error = "Expected 'Start' statement, rule or end of text";
	start->error = idError;
	startEnd->error = stmEndError;
	rule->error = "Expected production equality ':'";
	rhs->error = "Expected alternation '|', terminal id, statement end ';' or terminal start '[' ";
	terminalStart->error = idError;
	terminalEnd->error = "Expected terminal end '[' ";

	end->accept = true;
}

bool parse(const AlphabetType* text, Automata* automata, CFGTokenizer* tok, CfGrammar* grammar) {
	tok->reset();
	tok->bindSource(text);

	State* state = &(*automata)[0];
	while (!state->accept) {

		auto token = tok->readTok();

		if (token == Token::FAILED) {
			auto errorLocation = tok->getCursorPrev().get2DLocation();
			printf("Syntax error at (%llu, %llu)\n", errorLocation.line, errorLocation.character);
			return false;
		}

		auto tokVal = tok->extractVal();

		bool isTransition = false;

		for (auto transition : state->transitions) {
			if (transition->tok == token) {
				transition->action(grammar, tokVal);
				state = transition->target;
				isTransition = true;
				break;
			}
		}

		if (!isTransition) {
			// if no tok matched to any transition produce error
			auto errorLocation = tok->getCursorPrev().get2DLocation();
			printf("Parse error at (%llu, %llu) - %s", errorLocation.line, errorLocation.character, state->error.read());
			return false;
		}
	}

	return true;
}

struct tp::CfGrammarParserState {
	Automata automata;
	CFGTokenizer tokenizer;
};

CfGrammarParserState* CfGrammar::initializeCfGrammarParser() {
	auto state = new CfGrammarParserState();
	initializeTokenizer(&state->tokenizer);
	initAutomata(state->automata);
	return state;
}

void CfGrammar::deinitializeCfGrammarParser(CfGrammarParserState* state) {
	delete state;
}

void CfGrammar::parse(CfGrammarParserState* context, const int1* source) {
	::parse(source, &context->automata, &context->tokenizer, this);
}