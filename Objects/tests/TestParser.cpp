
#include "ObjectTests.hpp"

#include "parser/parser.h"

using namespace tp;
using namespace obj;

SUITE(Parser) {
	TEST(Basic) {
		objTestModule.initialize();

		{
			Parser parser;

			String stream = "";
			auto res = parser.parse(stream);

			CHECK(!res.isError);

			delete res.scope;
		}

		{
			Parser parser;

			String stream = "var i = true;";
			auto res = parser.parse(stream);

			CHECK(!res.isError);

			delete res.scope;
		}

		{
			Parser parser;

			String stream = "var i = true; print (i + 1) * 10;";
			auto res = parser.parse(stream);

			CHECK(!res.isError);

			delete res.scope;
		}

		objTestModule.deinitialize();
	}

	TEST(ErrorHandling) {
		objTestModule.initialize();

		{
			Parser parser;
			String stream = "var i = true; print (i + 1) * 10; invalidCharacter ";
			auto res = parser.parse(stream);

			CHECK(res.isError);

			delete res.scope;
		}

		objTestModule.deinitialize();
	}
}