#pragma once

#include "CommandLine.hpp"
#include "Map.hpp"

namespace tp {

	class CmdLineInterpreter {
	public:
		typedef bool (*CommandFunction)(void* customData, const CommandLine& args);

	private:
		struct Command {
			CommandLine* args;
			CommandFunction function;
			void* customData = nullptr;
		};

		Map<String, Command> mCommands;
		String mName = "InterpName";

	public:
		CmdLineInterpreter();
		~CmdLineInterpreter();

		void setName(const String& name) { mName = name; }
		[[nodiscard]] const String& getName() const { return mName; }

		void help(const CommandLine& args) const;
		void addCommand(const String& id, CommandLine* args, CommandFunction function, void* customData = nullptr);
		void run();
		bool processCommand(int1* command);
	};
}