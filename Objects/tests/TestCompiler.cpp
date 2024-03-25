
#include "ObjectTests.hpp"

#include "compiler/Functions.hpp"
#include "parser/Parser.hpp"
#include "core/Object.hpp"
#include "interpreter/Interpreter.hpp"
#include "primitives/InterpreterObject.hpp"
#include "primitives/LinkObject.hpp"
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

			NDO->destroy(method);

			assertNoLeaks();
		}

		{
			// with errors
			auto method = objects_api::create<MethodObject>();

			method->mScript->mReadable->val = "print undefinedVariable;";
			method->compile();

			NDO->destroy(method);

			assertNoLeaks();
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