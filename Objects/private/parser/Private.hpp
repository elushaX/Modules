
#pragma once

#include <lalr/Parser.hpp>
#include <lalr/string_literal.hpp>

#include "compiler/statement.h"
#include "compiler/expression.h"

#include "Tree.hpp"

struct UserNode {
	obj::BCgen::Expression* expression = nullptr;
	obj::BCgen::Statement* statement = nullptr;
	tp::Buffer<tp::String>* arguments = nullptr;

	UserNode(obj::BCgen::Statement* stm) :
		statement(stm) {}

	UserNode(obj::BCgen::Expression* expr) :
		expression(expr) {}

	UserNode(tp::Buffer<tp::String>* argList) :
		arguments(argList) {}

	UserNode() = default;

	obj::BCgen::Statement* releaseStm() {
		auto out = statement;
		statement = nullptr;
		return out;
	}

	void destroy() {
		delete statement;
		delete arguments;
		delete expression;
	}
};

typedef UserNode UserData;
typedef char SymbolType;

typedef std::basic_string<SymbolType>::iterator IteratorType;

extern const lalr::ParserStateMachine* oscript_parser_state_machine;

typedef lalr::Parser<IteratorType, UserData> LalrParser;

void bind(LalrParser& parser);
