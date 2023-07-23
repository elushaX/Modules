#include "NewPlacement.hpp"

#include "Tests.hpp"

using namespace tp;

static CommandLine createCmdLine() {
	CommandLine cmd({
			{ "bool", CommandLine::Arg::BOOL },
			{ "int", CommandLine::Arg::INT },
			{ "string", CommandLine::Arg::STR },
			{ "float", CommandLine::Arg::FLOAT },
	});
	return cmd;
}

TEST_DEF_STATIC(Simple) {
	auto cmd = createCmdLine();
	const char* args[] = { "false", "2", "'str'", "-0.15" };
	cmd.parse(4, args, true, 0);
}

TEST_DEF(CommandLine) {
	testSimple();
}
