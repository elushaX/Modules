
#pragma once

#include <lalr/Parser.hpp>
#include <lalr/string_literal.hpp>

#include "compiler/Statements.hpp"
#include "compiler/Expressions.hpp"

#include "Tree.hpp"

extern const lalr::ParserStateMachine* oscript_parser_state_machine;

namespace tp::obj {
	struct UserNode {
		obj::Expression* expression = nullptr;
		obj::Statement* statement = nullptr;
		tp::Buffer<std::string>* arguments = nullptr;

		UserNode(obj::Statement* stm) :
			statement(stm) {}

		UserNode(obj::Expression* expr) :
			expression(expr) {}

		UserNode(tp::Buffer<std::string>* argList) :
			arguments(argList) {}

		UserNode() = default;

		obj::Statement* releaseStm() {
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

	typedef lalr::Parser<IteratorType, UserData> LalrParser;

	void bind(LalrParser& parser);
}