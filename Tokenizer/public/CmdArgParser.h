#pragma once

#include "Tokenizer.hpp"

namespace tp {
	struct CmdArgParser {

		struct IntArg {
			alni mVal = 0;
			alni mDefault = 0;
			Range<alni> mAcceptingRange = { ENV_ALNI_MIN, ENV_ALNI_MAX };
		};

		struct FloatArg {
			alnf mVal = 0.f;
			alnf mDefault = 0.f;
			Range<alnf> mAcceptingRange = { ENV_ALNF_MIN, ENV_ALNF_MAX };
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
			string mId;
			enum Type { INT, FLOAT, BOOL, STR, FILE_IN } mType;
			union {
				IntArg mInt;
				FloatArg mFloat;
				BoolArg mBool;
				StringArg mStr;
				FileInputArg mFile;
			};

			bool mOptional = true;

			Arg(const Arg& arg);
			Arg(string id, Type type);
			Arg(string id, Range<alni> aAcceptingRange);
			Arg(string id, Range<alnf> aAcceptingRange);
			Arg(string id, Range<alni> aAcceptingRange, alni aDefault);
			Arg(string id, Range<alnf> aAcceptingRange, alnf aDefault);
			Arg(string id, bool aDefault);
			Arg(string id, const char* aDefault);
			~Arg();
		};

		struct Error {
			const char* mDescr = nullptr;
			Arg* mArg = nullptr;
			operator bool() { return mDescr != nullptr; }
		} mError;

		CmdArgParser(init_list<Arg> args);
		~CmdArgParser();

		bool parse(char argc, const char* argv[], bool logError = false);

		alni getInt(string id);
		alnf getFloat(string id);
		bool getBool(string id);
		string getString(string id);
		File& getFile(string id);

	private:
		enum class TokType { SPACE, INT, FLOAT, BOOL_FALSE, BOOL_TRUE, STR, NONE, FAILURE, END, } mType;
		typedef SimpleTokenizer<char, TokType, TokType::NONE, TokType::FAILURE, TokType::END> Tokenizer;

		Tokenizer mTokenizer;
		HashMap<Arg*, string> mArgs;
		List<Arg*> mArgsOrder;
		ualni mOptionals = 0;

		Arg& getArg(string id, Arg::Type type);
		void ErrInvalidArgCount();
		void ErrInvalidArgSyntax(Arg* arg);
		void ErrInvalidArgType(Arg* arg);
		void ErrFileNotExists(Arg* arg);
		void ErrFileCouldNotOpen(Arg* arg);
		void ErrValNotinRange(Arg* arg);
		void ErrLog();
		void ArgLog(Arg& arg);
		void initDefault(Arg& arg);
		void parseArg(Arg& arg, const char* src);
	};
};