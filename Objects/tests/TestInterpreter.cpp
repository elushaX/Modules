
#include "ObjectTests.hpp"

#include "compiler/Functions.hpp"
#include "interpreter/Interpreter.hpp"
#include "primitives/InterpreterObject.hpp"
#include "primitives/LinkObject.hpp"
#include "primitives/MethodObject.hpp"

using namespace tp;
using namespace obj;

auto script1 = R"(
class a {
	var string = "hello";
	method log(name) {
		<< self.string;
		<< name;
	}
}

method main() {
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
	print 101;
)";

SUITE(Interpreter) {
	TEST(Entry) {
		objTestModule.initialize();

		{
			auto method = objects_api::create<MethodObject>();
			auto interpreter = objects_api::create<InterpreterObject>();

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			method->mBytecodeLink->mReadable->val = script;
			method->compile();

			interpreter->exec();

			objects_api::destroy(interpreter);

			printf("\n");
		}

		objTestModule.deinitialize();
	}

	TEST(SimpleSave) {
		objTestModule.initialize();

		{
			auto method = objects_api::create<MethodObject>();
			auto interpreter = objects_api::create<InterpreterObject>();

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			method->mBytecodeLink->mReadable->val = script;
			method->compile();

			interpreter->exec();

			objects_api::save(interpreter, "interp.o");

			auto interpreterLoaded = objects_api::cast<InterpreterObject>(objects_api::load("interp.o"));

			interpreterLoaded->exec();

			objects_api::destroy(interpreterLoaded);
			objects_api::destroy(interpreter);

			printf("\n");
		}

		objTestModule.deinitialize();
	}

	TEST_OFF(Simple) {
		objTestModule.initialize();

		{
			auto compileStartCount = objects_api::getObjCount();

			auto method = objects_api::create<MethodObject>();
			auto interpreter = objects_api::create<InterpreterObject>();

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			auto exec = [&](const char* script) {
				method->mBytecodeLink->mReadable->val = script;
				method->compile();

				auto startCount = objects_api::getObjCount();
				interpreter->exec();

				if (objects_api::getObjCount() != startCount) {
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

			objects_api::destroy(interpreter);

			if (objects_api::getObjCount() != compileStartCount) {
				CHECK(false && "Mem leaks in compiler and interpreter");
			}
		}

		objTestModule.deinitialize();
	}

	TEST_OFF(Complex) {
		objTestModule.initialize();

		{
			auto method = objects_api::create<MethodObject>();
			auto interpreter = objects_api::create<InterpreterObject>();

			interpreter->getMember<LinkObject>("target method")->setLink(method);

			method->mBytecodeLink->mReadable->val = script1;
			method->compile();

			interpreter->exec();

			objects_api::destroy(interpreter);
		}

		objTestModule.deinitialize();
	}
}
