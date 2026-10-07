
#pragma once

#include "SimpleParser.hpp"

#include "compiler/Statements.hpp"
#include "compiler/Expressions.hpp"

namespace tp::obj {

	// The oscript grammar, expressed in the Unified Grammar format.
	// Each alternative carries an action tag that the AST builder dispatches on.
	extern const char* oscript_grammar;

	using OScriptParser = tp::SimpleParser<char>;
	using OScriptNode = const OScriptParser::StackItem*;
}
