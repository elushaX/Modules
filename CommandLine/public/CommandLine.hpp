#pragma once

#include "Tokenizer.hpp"
#include "Map.hpp"

namespace tp {

	// TODO : replace
	class File {

	};

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
			String mFilepath;
			File mFile;
		};

		struct Arg {
			String mId;
			enum Type { INT, FLOAT, BOOL, STR, FILE_IN } mType;
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

		CommandLine(const init_list<Arg>& args);
		~CommandLine();

		bool parse(char argc, const char* argv[], bool logError = true, ualni skipArgs = 1);

		alni getInt(const String& id);
		alnf getFloat(const String& id);
		bool getBool(const String& id);
		const String& getString(const String& id);
		File& getFile(const String& id);

	private:
		enum class TokType { SPACE, INT, FLOAT, BOOL_FALSE, BOOL_TRUE, STR, NONE, FAILURE, END, } mType;
		typedef SimpleTokenizer<char, TokType, TokType::NONE, TokType::FAILURE, TokType::END> Tokenizer;

		Tokenizer mTokenizer;
		Map<String, Arg*> mArgs;
		List<Arg*> mArgsOrder;
		ualni mOptionals = 0;

		Arg& getArg(const String& id, Arg::Type type);
		void ErrInvalidArgCount();
		void ErrInvalidArgSyntax(Arg* arg);
		void ErrInvalidArgType(Arg* arg);
		void ErrFileNotExists(Arg* arg);
		void ErrFileCouldNotOpen(Arg* arg);
		void ErrValNotinRange(Arg* arg);
		void ErrLog();
		static void ArgLog(Arg& arg);
		static void initDefault(Arg& arg);
		void parseArg(Arg& arg, const char* src);
	};
};