
#include "ObjectTests.hpp"

#include "compiler/Functions.hpp"
#include "core/Object.hpp"
#include "interpreter/Interpreter.hpp"
#include "primitives/InterpreterObject.hpp"
#include "primitives/LinkObject.hpp"
#include "primitives/MethodObject.hpp"

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

SUITE(Interpreter) {
	TEST(Entry) {
		objTestModule.initialize();
		
		{
			auto method = NDO_CAST(MethodObject, NDO->create("method"));
			auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			method->mScript->mReadable->val = script;
			method->compile();

			interpreter->exec();

			NDO->destroy(interpreter);

			printf("\n");
		}

		objTestModule.deinitialize();
	}

	TEST(SimpleSave) {
		objTestModule.initialize();

		{
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

		objTestModule.deinitialize();
	}

	TEST_OFF(Simple) {
		objTestModule.initialize();

		{
			auto compileStartCount = getObjCount();

			auto method = NDO_CAST(MethodObject, NDO->create("method"));
			auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			auto exec = [&](const char* script) {
				method->mScript->mReadable->val = script;
				method->compile();

				auto startCount = getObjCount();
				interpreter->exec();

				if (getObjCount() != startCount) {
					CHECK(false && "Mem leaks in interpreter");
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
				CHECK(false && "Mem leaks in compiler and interpreter");
			}
		}

		objTestModule.deinitialize();
	}

	TEST(Complex) {
		objTestModule.initialize();

		{
			auto method = NDO_CAST(MethodObject, NDO->create("method"));
			auto interpreter = NDO_CAST(InterpreterObject, NDO->create("interpreter"));

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			method->mScript->mReadable->val = script1;
			method->compile();

			interpreter->exec();

			NDO->destroy(interpreter);
		}

		objTestModule.deinitialize();
	}
}
