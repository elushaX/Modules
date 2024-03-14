
#include "ObjectTests.hpp"

#include "compiler/function.h"
#include "core/object.h"
#include "interpreter/interpreter.h"
#include "primitives/interpreterobject.h"
#include "primitives/linkobject.h"
#include "primitives/methodobject.h"

using namespace tp;
using namespace obj;

TEST_DEF_STATIC(Basic) {
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
}

TEST_DEF_STATIC(ErrorHandling) {
	Parser parser;

	String stream = "var i = true; print (i + 1) * 10; invalidCharacter ";
	auto res = parser.parse(stream);

	TEST(res.isError);

	delete res.scope;
}

TEST_DEF(Compiler) {
	if (objTestModule.initialize()) {
		testBasic();
		testErrorHandling();
		objTestModule.deinitialize();
	}
}
