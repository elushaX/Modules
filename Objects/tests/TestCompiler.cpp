
#include "ObjectTests.hpp"

#include "compiler/function.h"
#include "parser/parser.h"
#include "core/object.h"
#include "interpreter/interpreter.h"
#include "primitives/interpreterobject.h"
#include "primitives/linkobject.h"
#include "primitives/methodobject.h"

using namespace tp;
using namespace obj;

SUITE(Compiler) {
	TEST(Basic) {
		objTestModule.initialize();

		{	
			// no errors
			auto method = NDO_CAST(MethodObject, NDO->create("method"));

			method->mScript->mReadable->val = "print 1 * 20 + 10;";
			method->compile();

			NDO->destroy(method);

			assertNoLeaks();
		}

		{
			// with errors
			auto method = NDO_CAST(MethodObject, NDO->create("method"));

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