
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
	print false;
)";

TEST_DEF_STATIC(Entry) {
	auto method = NDO_CAST(MethodObject, NDO->create("method"));
	auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

	interpreter->getMember<LinkObject>("target method")->setLink(method);

	method->mScript->mReadable->val = script;
	method->compile();

	interpreter->exec();

	NDO->destroy(interpreter);

	printf("\n");
}

TEST_DEF_STATIC(SimpleSave) {
	auto method = NDO_CAST(MethodObject, NDO->create("method"));
	auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

	interpreter->getMember<LinkObject>("target method")->setLink(method);

	method->mScript->mReadable->val = script;
	method->compile();

	interpreter->exec();

	NDO->save(interpreter, "interp.o");

	auto interpreterLoaded = NDO_CAST(InterpreterObject, NDO->load("interp.o"));

	interpreterLoaded->exec();

	NDO->destroy(interpreterLoaded);
	NDO->destroy(interpreter);

	printf("\n");
}

TEST_DEF_STATIC(Simple) {
	auto compileStartCount = getObjCount();

	auto method = NDO_CAST(MethodObject, NDO->create("method"));
	auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

	interpreter->getMember<LinkObject>("target method")->setLink(method);

	auto exec = [&](const char* script){
		method->mScript->mReadable->val = script;
		method->compile();

		auto startCount = getObjCount();
		interpreter->exec();

		if (getObjCount() != startCount) {
			TEST(false && "Mem leaks in interpreter");
		}

		printf("\n");
	};

	exec("10 + 15;");
	exec("print 10 + 15 * 10;");
	exec("print 10 + 15 * (10 + 10) + 5;");
	exec("print 10 + 15;");
	exec("print (10 + 15) * 20;");
	exec("var k : int;");
	exec("var k : int; print k;");

	NDO->destroy(interpreter);

	if (getObjCount() != compileStartCount) {
		TEST(false && "Mem leaks in compiler and interpreter");
	}
}

TEST_DEF_STATIC(Complex) {
	auto method = NDO_CAST(MethodObject, NDO->create("method"));
	auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

	interpreter->getMember<LinkObject>("target method")->setLink(method);

	method->mScript->mReadable->val = script1;
	method->compile();

	interpreter->exec();

	NDO->destroy(interpreter);
}

TEST_DEF(Interpreter) {
	testEntry();
	testSimple();
	testSimpleSave();
	// testComplex();
}
