

#include <parser/parser.h>

// #include "compiler/function.h"

#include <lalr/Parser.hpp>
#include <lalr/string_literal.hpp>

using namespace obj;

struct ASTNode {
	BCgen::Expression* expression = nullptr;
	BCgen::Statement* statement = nullptr;
};

extern const lalr::ParserStateMachine* oscript_parser_state_machine;
typedef void* UserData;
typedef char SymbolType;

typedef std::basic_string<SymbolType>::iterator IteratorType;

namespace action {
	static UserData expr_bool(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return new BCgen::ExpressionConst(nodes[0].lexeme() == "true");
	}

	static UserData expr_int(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return {};
	}

	static UserData expr_float(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return {};
	}

	static UserData expr_string(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return {};
	}

	static UserData stm_log(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		auto expr = (BCgen::Expression*) (start[1]);
		return new BCgen::StatementPrint(expr);
	}

	static UserData stm_defVar(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return new BCgen::StatementLocalDef(nodes[1].lexeme().c_str(), new BCgen::ExpressionConst(false));
	}

	static UserData stm_scope_append(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		auto scope = (BCgen::StatementScope*) start[0];
		auto stm = (BCgen::Statement*) start[2];
		scope->mStatements.append(stm);
		return scope;
	}

	static UserData stm_scope_create(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		auto stm = (BCgen::Statement*) start[0];
		return new BCgen::StatementScope({ stm }, true);
	}

	static UserData scope(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return start[1];
	}

	static UserData tmp(const UserData* start, const lalr::ParserNode<SymbolType>* nodes, size_t length) {
		return *start;
	}
}

Parser::Result Parser::parse(const tp::String& stream) {

	lalr::ErrorPolicy errorPolicy;

	lalr::Parser<IteratorType, UserData> parser(oscript_parser_state_machine, &errorPolicy);

	parser.set_lexer_action_handler("string", &lalr::string_literal<IteratorType>);

	parser.set_action_handler("expr_bool", &action::expr_bool);
	parser.set_action_handler("expr_int", &action::expr_int);
	parser.set_action_handler("expr_float", &action::expr_float);
	parser.set_action_handler("expr_string", &action::expr_string);

	parser.set_action_handler("stm_defVar", &action::stm_defVar);
	parser.set_action_handler("stm_log", &action::stm_log);

	parser.set_action_handler("stm_scope_create", &action::stm_scope_create);
	parser.set_action_handler("stm_scope_append", &action::stm_scope_append);

	parser.set_action_handler("scope", &action::scope);

	parser.set_action_handler("tmp", &action::tmp);

	std::string streamStd(stream.read());

	LALR_ASSERT(parser.valid());

	parser.parse(streamStd.begin(), streamStd.end());

	parser.accepted();
	parser.full();

	auto scope = (BCgen::StatementScope*) parser.user_data();

	return { nullptr, scope };
}