#pragma once

#include "LocalConnection.hpp"
#include "Map.hpp"
#include "Tokenizer.hpp"


namespace tp {

	extern ModuleManifest gModuleCommandLine;

	struct CommandLine {

		struct IntArg {
			alni mVal = 0;
			alni mDefault = 0;
			Range<alni> mAcceptingRange = { -(std::numeric_limits<alni>::max() - 1), std::numeric_limits<alni>::max() };
		};

		struct FloatArg {
			alnf mVal = 0.f;
			alnf mDefault = 0.f;
			Range<alnf> mAcceptingRange = { -(std::numeric_limits<alnf>::max() - 1), std::numeric_limits<alnf>::max() };
		};

		struct BoolArg {
			bool mFlag = false;
			bool mDefault = false;
		};

		struct StringArg {
			String mStr;
			String mDefault;
		};

		struct FileInputArg {
			LocalConnection::Location mFileLocation;
		};

		struct Arg {
			String mId;
			enum Type { INT, FLOAT, BOOL, STR, FILE_IN, NONE } mType = NONE;
			union {
				IntArg mInt;
				FloatArg mFloat;
				BoolArg mBool;
				StringArg mStr;
				FileInputArg mFile;
			};

			bool mOptional = true;
			bool mIsPassed = false;

			Arg(const Arg& arg);
			Arg(const String& id, Type type);
			Arg(const String& id, const Range<alni>& aAcceptingRange);
			Arg(const String& id, const Range<alnf>& aAcceptingRange);
			Arg(const String& id, const Range<alni>& aAcceptingRange, alni aDefault);
			Arg(const String& id, const Range<alnf>& aAcceptingRange, alnf aDefault);
			Arg(const String& id, bool aDefault);
			Arg(const String& id, const char* aDefault);
			~Arg();
		};

		struct Error {
			const char* mDescr = nullptr;
			Arg* mArg = nullptr;
			operator bool() { return mDescr != nullptr; }
		} mError;

		CommandLine(const InitialierList<Arg>& args);
		~CommandLine();

		void resetError() { mError.mDescr = nullptr; }

		bool parse(char argc, const char* argv[], bool logError = true, ualni skipArgs = 1);
		bool parse(const char* args, bool logError = true);

		void help() const;

		[[nodiscard]] alni getInt(const String& id) const;
		[[nodiscard]] alnf getFloat(const String& id) const;
		[[nodiscard]] bool getBool(const String& id) const;
		[[nodiscard]] const String& getString(const String& id) const;
		[[nodiscard]] const LocalConnection::Location& getFile(const String& id) const;

		const CommandLine& operator=(const CommandLine&) = delete;

	private:
		enum class TokType {
			SPACE,
			INT,
			FLOAT,
			BOOL_FALSE,
			BOOL_TRUE,
			STR,
			NONE,
			FAILURE,
			END,
		};
		typedef SimpleTokenizer<char, TokType, TokType::NONE, TokType::FAILURE, TokType::END> Tokenizer;

		Tokenizer mTokenizer;
		Map<String, Arg*> mArgs;
		List<Arg*> mArgsOrder;
		ualni mOptionals = 0;

		enum class ArgTokType {
			SPACE,
			ARG,
			NONE,
			FAILURE,
			END,
		};
		SimpleTokenizer<char, ArgTokType, ArgTokType::NONE, ArgTokType::FAILURE, ArgTokType::END> mArgumentTokenizer;

		Arg& getArg(const String& id, Arg::Type type);
		[[nodiscard]] const Arg& getArg(const String& id, Arg::Type type) const;

		void ErrInvalidArgCount();
		void ErrInvalidArgSyntax(Arg* arg);
		void ErrInvalidArgType(Arg* arg);
		void ErrFileNotExists(Arg* arg);
		void ErrFileCouldNotOpen(Arg* arg);
		void ErrValNotinRange(Arg* arg);
		void ErrLog();
		static void logArg(const Arg& arg);
		static void initDefault(Arg& arg);
		void parseArg(Arg& arg, const char* src);
	};
}
