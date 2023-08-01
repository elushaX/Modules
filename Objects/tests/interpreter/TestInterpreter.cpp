
#include "NewPlacement.hpp"

#include "Testing.hpp"

#include "core/object.h"
#include "primitives/methodobject.h"
#include "primitives/interpreterobject.h"
#include "primitives/linkobject.h"
#include "compiler/function.h"
#include "interpreter/interpreter.h"

using namespace tp;
using namespace obj;

TEST_DEF_STATIC(Simple) {

	auto method = NDO_CAST(MethodObject, NDO->create("method"));
	auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

	interpreter->getMember<LinkObject>("target method")->setLink(method);

	method->mScript->mReadable->val = "<< 3;";
	method->compile();

	interpreter->exec();

	NDO->save(interpreter, "interp.o");

	auto interpreterLoaded = NDO_CAST(InterpreterObject, NDO->load("interp.o"));

	interpreterLoaded->exec();

	NDO->destroy(interpreter);
	NDO->destroy(interpreterLoaded);
}

TEST_DEF(Interpreter) {
	testSimple();
}