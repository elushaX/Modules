
#include "NewPlacement.hpp"

#include "Parser.hpp"
#include "CommandLine.hpp"

using namespace tp;

void run(const String& source) {
	auto state = CfGrammar::initializeCfGrammarParser();

	CfGrammar grammar;

	if (!grammar.parse(state, source)) {
		printf("Parsing is failed\n");
		CfGrammar::deinitializeCfGrammarParser(state);
		return;
	}

	if (!grammar.compile()) {
		printf("Compilation is failed\n");
		CfGrammar::deinitializeCfGrammarParser(state);
		return;
	}

	printf("Grammar accepted.\n");

	List<CfGrammar::Sentence> sentences;
	grammar.generateSentences(sentences);

	printf("Example sentences formed from grammar: \n");
	for (auto sentence : sentences) tp::CfGrammar::printSentence(sentence.data());

	CfGrammar::deinitializeCfGrammarParser(state);
}

int main(int argc, const char* argv[]) {
	tp::ModuleManifest* deps[] = { &tp::gModuleParser, &tp::gModuleCommandLine, nullptr };
	tp::ModuleManifest testModule("CommandLineTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	{
		printf("Arguments given: ");
		for (auto idx = 0; idx < argc; idx++) {
			printf("[%s]", argv[idx]);
		}
		printf("\n");

		CommandLine cmd = {
				{ "grammar", CommandLine::Arg::STR },
		};

		if (!cmd.parse((char) argc, argv, true, 1)) {
			return 1;
		}

		auto location = cmd.getString("grammar");

		LocalConnection file;
		String grammar;

		file.connect(LocalConnection::Location(location), LocalConnection::Type(true));

		if (!file.getConnectionStatus().isOpened()) {
			printf("Cant open file\n");
			return 1;
		}

		auto size = file.size();
		grammar.resize((String::Index) size);
		file.readBytes(grammar.write(), size);


		run(grammar);

		file.disconnect();
	}

	testModule.deinitialize();
	return 0;
}