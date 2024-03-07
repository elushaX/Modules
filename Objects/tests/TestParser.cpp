
#include "Testing.hpp"

#include "parser/parser.h"

using namespace tp;
using namespace obj;

TEST_DEF_STATIC(Basic) {
	Parser parser;

	String stream = "{ var i; print false; }";
	auto res = parser.parse(stream);
}

TEST_DEF(Parser) { testBasic(); }
