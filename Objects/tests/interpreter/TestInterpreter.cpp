
#include "NewPlacement.hpp"

#include "Testing.hpp"

#include "compiler/function.h"
#include "core/object.h"
#include "interpreter/interpreter.h"
#include "primitives/interpreterobject.h"
#include "primitives/linkobject.h"
#include "primitives/methodobject.h"

using namespace tp;
using namespace obj;

auto script1 = R"(
class A {
	var string = "hello";
	def log(name) {
		<< self.string;
		<< name;
	}
}

def main() {
	var a = new A();
	a.log(" user ");
}

main();

var i = 10;

if (i == 10) {
	while (i > 0) {
		<< i;
		i = i - 1;
	}
} else {
	<< " no ";
}

)";

auto script = R"(
var i = 10;
)";

TEST_DEF_STATIC(Complex) {
	auto method = NDO_CAST(MethodObject, NDO->create("method"));
	auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

	interpreter->getMember<LinkObject>("target method")->setLink(method);

	method->mScript->mReadable->val = script1;
	method->compile();

	interpreter->exec();

	NDO->destroy(interpreter);
}

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

	NDO->destroy(interpreterLoaded);
	NDO->destroy(interpreter);
}

TEST_DEF(Interpreter) {
	testSimple();
	// testComplex();
}