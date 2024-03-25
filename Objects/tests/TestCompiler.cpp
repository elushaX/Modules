
#include "ObjectTests.hpp"

#include "compiler/Functions.hpp"
#include "interpreter/Interpreter.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

SUITE(Compiler) {
	TEST(Basic) {
		objTestModule.initialize();

		{
			// no errors
			auto method = objects_api::create<MethodObject>();

			method->mScript->mReadable->val = "print 1 * 20 + 10;";
			method->compile();

			objects_api::destroy(method);

			objects_api::assertNoLeaks();
		}

		{
			// with errors
			auto method = objects_api::create<MethodObject>();

			method->mScript->mReadable->val = "print undefinedVariable;";
			method->compile();

			objects_api::destroy(method);

			objects_api::assertNoLeaks();
		}

		objTestModule.deinitialize();
	}

	/*
	TEST(ErrorHandling) {
		objTestModule.initialize();

		obj::Parser parser;

		String stream = "var i = true; print (i + 1) * 10; invalidCharacter ";
		auto res = parser.parse(stream);

		CHECK(res.isError);

		delete res.scope;

		objTestModule.deinitialize();
	}
	*/
}