#include "NewPlacement.hpp"

#include "CmdLineInterpreter.hpp"
#include "Tests.hpp"

using namespace tp;

TEST_DEF_STATIC(Simple) {

	struct TestStruct {
		int val = 0;

		static bool print(TestStruct* self, const CommandLine& args) {
			printf("Test Val - %i\n", (int) args.getBool("bool"));
			return false;
		}
	};

	TestStruct self;

	CmdLineInterpreter interp;

	interp.addCommand("test", new CommandLine{ { "bool", CommandLine::Arg::BOOL } }, (CmdLineInterpreter::CommandFunction) TestStruct::print);

	printf("test false\n", stdin);
	printf("help\n", stdin);
	printf("help test\n", stdin);
	printf("exit\n", stdin);

	interp.run();
}

TEST_DEF(Interpreter) {
	// testSimple();
}
