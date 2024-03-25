
#include "ObjectTests.hpp"
#include "parser/Parser.hpp"

using namespace tp;
using namespace obj;

SUITE(Parser) {
	TEST(Basic) {
		objTestModule.initialize();

		{
			Parser parser;

			std::string stream = "";
			auto res = parser.parse(stream);

			CHECK(!res.isError);

			delete res.scope;
		}

		{
			Parser parser;

			std::string stream = "var i = true;";
			auto res = parser.parse(stream);

			CHECK(!res.isError);

			delete res.scope;
		}

		{
			Parser parser;

			std::string stream = "var i = true; print (i + 1) * 10;";
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
			std::string stream = "var i = true; print (i + 1) * 10; invalidCharacter ";
			auto res = parser.parse(stream);

			CHECK(res.isError);

			delete res.scope;
		}

		objTestModule.deinitialize();
	}
}