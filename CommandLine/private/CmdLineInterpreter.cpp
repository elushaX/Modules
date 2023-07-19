#include "CmdLineInterpreter.hpp"

#define MAX_LINE_LENGTH 1024

#include <cstdio>
#include <cstring>

using namespace tp;

static bool helpFunction(CmdLineInterpreter* self, const CommandLine& args) {
	self->help(args);
	return true;
}

void CmdLineInterpreter::help(const CommandLine& args) const {
	auto commandName = args.getString("CommandName");
	auto allCommands = commandName == "_none_";

	if (allCommands) {
		printf("Commands:\n");
		for (auto command : mCommands) {
			printf("  %s\n", command->key.read());
		}
		return;
	}

	auto idx = mCommands.presents(commandName);
	if (!idx) {
		printf("Command '%s' not found\n", commandName.read());
		return;
	}

	printf("Command '%s'\n", commandName.read());
	auto command = &mCommands.getSlotVal(idx);
	command->args->help();
}

CmdLineInterpreter::CmdLineInterpreter() {
	auto helpArgs = new CommandLine{
		{ "CommandName", "_none_" }
	};

	addCommand("help", helpArgs, (CommandFunction) helpFunction, this);
}


void CmdLineInterpreter::addCommand(const String& id, CommandLine* args, CommandFunction function, void* customData) {
	ASSERT(!mCommands.presents(id))
	mCommands.put(id, { args, function, customData });
}

void CmdLineInterpreter::run() {
	char buffer[MAX_LINE_LENGTH];
	printf("Command Line Interpreter (or type 'exit' to quit):\n");

	while (true) {
		printf("%s >> ", mName.read());

		// Read input from the user
		if (fgets(buffer, MAX_LINE_LENGTH, stdin) == nullptr) {
			printf("Error reading input.\n");
			return;
		}

		// Remove the trailing newline character if it exists
		auto length = strlen(buffer);
		if (length > 0 && buffer[length - 1] == '\n') {
			buffer[length - 1] = '\0';
		}

		// Check if the user wants to exit
		if (strcmp(buffer, "exit") == 0) {
			return;
		}

		// Process the input here
		processCommand(buffer);
	}
}

bool CmdLineInterpreter::processCommand(int1* commandString) {
	ualni commandLen = 0;
	for (auto i = commandString; *i != 0; i++) {
		if (*i == ' ') break;
		commandLen++;
	}

	commandString[commandLen] = 0;
	auto args = commandString + commandLen + 1;

	auto idx = mCommands.presents(commandString);
	if (!idx) {
		printf("ERROR: Command '%s' is not found\n", commandString);
		printf("Please execute 'help' for more information\n\n");
		return true;
	}

	auto command = &mCommands.getSlotVal(idx);

	command->args->parse(args);

	if (command->args->mError) {
		printf("ERROR: Command call '%s' has invalid arguments\n", commandString);
		command->args->resetError();
		return true;
	}

	if (!command->function(command->customData, *command->args)) {
		printf("ERROR: Command call '%s' failed\n", commandString);
		return true;
	}

	return true;
}

CmdLineInterpreter::~CmdLineInterpreter() {
	for (auto cmd : mCommands) {
		delete cmd->val.args;
	}
}