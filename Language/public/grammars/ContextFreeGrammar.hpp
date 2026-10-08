
#pragma once

#include "Buffer.hpp"
#include "Map.hpp"
#include "Strings.hpp"

namespace tp {

	class ContextFreeGrammar {
	public:
		struct Arg {
			friend class Rule;

		public:
			Arg() = default;
			explicit Arg(const String& id, bool terminal = true, bool epsilon = false);

		public:
			bool operator==(const Arg& in) const;
			[[nodiscard]] const String& getId() const;
			[[nodiscard]] bool isTerminal() const { return mIsTerminal; }
			[[nodiscard]] bool isEpsilon() const { return mIsEpsilon; }

		private:
			String mId;
			bool mIsTerminal = false;
			bool mIsEpsilon = false;
		};

		class Rule {
		public:
			Rule() = default;
			explicit Rule(const String& id) { mId = id; }
			Rule(const String& id, const InitialierList<Arg>& args);

		public:
			bool operator==(const Rule& in) const;
			[[nodiscard]] bool isProductive() const;
			[[nodiscard]] const String& getId() const { return mId; }
			[[nodiscard]] const Buffer<Arg>* getArgs() const { return &mArgs; }
			void addArg(const Arg& arg) { mArgs.append(arg); }

		private:
			String mId;
			Buffer<Arg> mArgs;
		};

	public:
		ContextFreeGrammar() = default;

	public:
		void addRule(const Rule& rule);
		void addRule(const String& id, const InitialierList<Arg>& args);
		Rule* addRule(const String& id) {
			mRules.append(Rule(id));
			return &mRules.last();
		}
		void setStart(const String& startRule);
		[[nodiscard]] const Buffer<Rule>* getRules() const { return &mRules; }
		[[nodiscard]] const String& getStartTerminal() const { return mStartTerminal; }

		void setTerminalPrecedence(const String& name, ualni level, alni assoc) {
			mTerminalPrecedence.put(name, level);
			mTerminalAssoc.put(name, assoc);
		}
		[[nodiscard]] const Map<String, ualni>* getTerminalPrecedence() const { return &mTerminalPrecedence; }
		[[nodiscard]] const Map<String, alni>* getTerminalAssoc() const { return &mTerminalAssoc; }

	public:
		Buffer<Rule> mRules;
		Map<String, ualni> mTerminalPrecedence;
		Map<String, alni> mTerminalAssoc;
		String mStartTerminal;
		bool mIsLooped = false;
	};
}

#include "grammars/RegularGrammar.ipp"
