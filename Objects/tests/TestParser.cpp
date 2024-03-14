
#include "ObjectTests.hpp"

#include "parser/parser.h"

using namespace tp;
using namespace obj;

TEST_DEF_STATIC(Basic) {
	{
		Parser parser;

		String stream = "";
		auto res = parser.parse(stream);

		TEST(!res.isError);

		delete res.scope;
	}

	{
		Parser parser;

		String stream = "var i = true;";
		auto res = parser.parse(stream);

		TEST(!res.isError);

		delete res.scope;
	}

	{
		Parser parser;

		String stream = "var i = true; print (i + 1) * 10;";
		auto res = parser.parse(stream);

		TEST(!res.isError);

		delete res.scope;
	}
}

TEST_DEF_STATIC(ErrorHandling) {
	Parser parser;

	String stream = "var i = true; print (i + 1) * 10; invalidCharacter ";
	auto res = parser.parse(stream);

	TEST(res.isError);

	delete res.scope;
}

TEST_DEF(Parser) {
	if (objTestModule.initialize()) {
		
		testBasic();
		testErrorHandling();

		objTestModule.deinitialize();
	}
}
