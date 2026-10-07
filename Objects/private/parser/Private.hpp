
#pragma once

#include "SimpleParser.hpp"

#include "compiler/Statements.hpp"
#include "compiler/Expressions.hpp"

namespace tp::obj {

	using OScriptParser = tp::SimpleParser<char>;
	using OScriptNode = const OScriptParser::StackItem*;
}
