#pragma once

#include "CfGrammar.hpp"

namespace tp {
	class CLR {
	public:
		struct ASTNode {
			const CfGrammar::Rule* production = nullptr;
			Map<String, String> terminals;
		};

		class Automation {
		public:
			class State {

			};
		};

		class TerminalStream {
		public:
			typedef ualni TerminalID;
		public:
			TerminalStream() = default;
			virtual TerminalID getNextTerminal() = 0;
		};

	public:
		struct BuildError {
			String description;
		};

		struct ParseError {
			const Automation::State* state = nullptr;
			String::Index location = 0;
		};

	private:
		class SententialStack {

		};

	public:
		CLR() = default;

		void setGrammar(const CfGrammar& grammar);
		void setTerminal(const String& name, TerminalStream::TerminalID id);
		void build();

		void parse(TerminalStream* stream, List<ASTNode>& out, ASTNode** root);

		[[nodiscard]] bool isBuild() { return mState == ParserState::BUILD; }
		[[nodiscard]] const BuildError& getBuildError() { return mBuildError; }
		[[nodiscard]] const ParseError& getParseError() { return mParseError; }

	private:
		enum class ParserState { NONE, BUILD, FAILED } mState = ParserState::NONE;

		ParseError mParseError;
		BuildError mBuildError;

		SententialStack mStack;
		Automation mAutomation;
	};
}