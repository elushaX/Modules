
#include "Testing.hpp"

#include "parser/parser.h"

using namespace tp;
using namespace obj;

TEST_DEF_STATIC(Basic) {
	Parser parser;

	String stream = "{ var i = true; print (i + 1) * 10; }";
	auto res = parser.parse(stream);
}

TEST_DEF(Parser) { testBasic(); }
