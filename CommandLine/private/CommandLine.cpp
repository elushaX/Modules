
#include "CommandLine.hpp"

using namespace tp;

const char* regexSpace = "\n|\t| |\r";
const char* regexFalse = "N|n|(False)|(false)";
const char* regexTrue = "Y|y|(True)|(true)";
const char* regexInt = "((\\-)|(\\+))?[0-9]+";
const char* regexFloat = R"(((\-)|(\+))?([0-9]+)(\.)([0-9]*)?)";
const char* regexString = "'{'-'}*'";

CommandLine::Arg::Arg(const Arg& arg) {
	mId = arg.mId;
	switch (arg.mType) {
	case CommandLine::Arg::INT: mType = INT;	mInt = arg.mInt; break;
	case CommandLine::Arg::FLOAT: mType = FLOAT; mFloat = arg.mFloat; break;
	case CommandLine::Arg::BOOL: mType = BOOL; mBool = arg.mBool; break;
	case CommandLine::Arg::STR: mType = STR; new (&mStr) StringArg(); mStr = arg.mStr; break;
	case CommandLine::Arg::FILE_IN: mType = FILE_IN;	new (&mFile) FileInputArg(); mFile = arg.mFile; break;
	}
	mOptional = arg.mOptional;
}

CommandLine::Arg::Arg(const String& id, Type type) {
	mId = id;
	switch (type) {
		case CommandLine::Arg::INT: mType = INT; new (&mInt) IntArg(); break;
		case CommandLine::Arg::FLOAT: mType = FLOAT; new (&mFloat) FloatArg(); break;
		case CommandLine::Arg::BOOL: mType = BOOL; new (&mBool) BoolArg(); break;
		case CommandLine::Arg::STR: mType = STR; new (&mStr) StringArg(); break;
		case CommandLine::Arg::FILE_IN: mType = FILE_IN;	new (&mFile) FileInputArg(); break;
	}
	mOptional = false;
}

CommandLine::Arg::Arg(const String& id, const Range<alni>& aAcceptingRange) {
	mId = id;
	mType = FLOAT;
	mInt = { 0, 0, aAcceptingRange };
	mOptional = false;
}

CommandLine::Arg::Arg(const String& id, const Range<alnf>& aAcceptingRange) {
	mId = id;
	mType = FLOAT;
	mFloat = { 0, 0, aAcceptingRange };
	mOptional = false;
}

CommandLine::Arg::Arg(const String& id, const Range<alni>& aAcceptingRange, alni aDefault) {
	mId = id;
	mType = INT;
	mInt = { 0, aDefault, aAcceptingRange };
}

CommandLine::Arg::Arg(const String& id, const Range<alnf>& aAcceptingRange, alnf aDefault) {
	mId = id;
	mType = FLOAT;
	mFloat = { 0, aDefault, aAcceptingRange };
}

CommandLine::Arg::Arg(const String& id, bool aDefault) {
	mId = id;
	mType = BOOL;
	mBool.mDefault = aDefault;
}

CommandLine::Arg::Arg(const String& id, const char* aDefault) {
	mId = id;
	mType = STR;
	new (&mStr) StringArg();
	mStr.mDefault = aDefault;
}

CommandLine::Arg::~Arg() {
	switch (mType) {
		case CommandLine::Arg::STR:
			mStr.~StringArg();
			break;
		case CommandLine::Arg::FILE_IN:
			mFile.~FileInputArg();
			break;
		default:
			break;
	}
}

CommandLine::CommandLine(const init_list<Arg>& args) {
	bool optional_start = false;

	for (auto& arg : args) {
		ASSERT(!mArgs.presents(arg.mId) && "Argument Redefinition")

		auto copy_arg = new Arg(arg);

		mArgs.put(arg.mId, copy_arg);
		mArgsOrder.pushBack(mArgs.get(arg.mId));

		if (arg.mOptional) {
			mOptionals++;
		}

		if (optional_start) {
			ASSERT(arg.mOptional && "Not Optional Argument After Optionals")
		}
		else if (arg.mOptional) {
			optional_start = true;
		}
	}

	mTokenizer.build({
		{ regexSpace, TokType::SPACE },
		{ regexFalse, TokType::BOOL_FALSE },
		{ regexTrue, TokType::BOOL_TRUE },
		{ regexInt, TokType::INT },
		{ regexFloat, TokType::FLOAT },
		{ regexString, TokType::STR },
		});

	ASSERT(mTokenizer.isBuild() && "Internal Error")
}

CommandLine::~CommandLine() {
	for (auto arg : mArgsOrder) {
		delete arg.data();
	}
}

bool CommandLine::parse(char argc, const char* argv[], bool logError, ualni skipArgs) {
	// discard some arguments
	argc -= (char) skipArgs;
	argv += skipArgs;

	if (argc < mArgs.size() - mOptionals) {
		ErrInvalidArgCount();
		if (logError) {
			ErrLog();
		}
		return false;
	}

	ualni idx = 0;
	for (auto arg : mArgsOrder) {
		if (idx < argc) {
			parseArg(*arg.data(), argv[idx]);
		} else {
			initDefault(*arg.data());
		}
		if (mError) {
			if (logError) ErrLog();
			return false;
		}
		idx++;
	}

	return true;

	//set_working_dir();
}

alni CommandLine::getInt(const String& id) { auto& arg = getArg(id, Arg::INT); return arg.mInt.mVal; }
alnf CommandLine::getFloat(const String& id) { auto& arg = getArg(id, Arg::FLOAT); return arg.mFloat.mVal; }
bool CommandLine::getBool(const String& id) { auto& arg = getArg(id, Arg::BOOL); return arg.mBool.mFlag; }
const String& CommandLine::getString(const String& id) { auto& arg = getArg(id, Arg::STR); return arg.mStr.mStr; }
File& CommandLine::getFile(const String& id) { auto& arg = getArg(id, Arg::FILE_IN); return arg.mFile.mFile; }


CommandLine::Arg& CommandLine::getArg(const String& id, Arg::Type type) {
	auto idx = mArgs.presents(id);
	ASSERT(idx && "Invalid Id")
	auto& arg = mArgs.getSlotVal(idx);
	ASSERT(arg->mType == type && "Invalid Type Requested")
	return *arg;
}

void CommandLine::ErrInvalidArgCount() { mError = { "Invalid Number Of Arguments Passed", nullptr }; }
void CommandLine::ErrInvalidArgSyntax(Arg* arg) { mError = { "Invalid Syntax Of Argument", arg }; }
void CommandLine::ErrInvalidArgType(Arg* arg) { mError = { "Invalid Type Of Argument", arg }; }
void CommandLine::ErrFileNotExists(Arg* arg) { mError = { "File Not Exists", arg }; }
void CommandLine::ErrFileCouldNotOpen(Arg* arg) { mError = { "Could Not Open File", arg }; }
void CommandLine::ErrValNotinRange(Arg* arg) { mError = { "Value Not In Range", arg }; }

void CommandLine::ErrLog() {
	printf("\nERROR : Invalid Command Line\n");

	if (mError.mArg) {
		alni idx = 0;
		for (auto arg : mArgsOrder) {
			if (mError.mArg == arg.data()) break;
			idx++;
		}

		printf("\nArgument: %lli\n", idx);
		ArgLog(*mError.mArg);
	}

	printf("Error Description: %s. \n", mError.mDescr);

	printf("\nExpected Arguments are: \n");
	ualni idx = 0;
	for (auto arg : mArgsOrder) {
		printf(" -- %lli -- \n", idx);
		ArgLog(*arg.data());
		idx++;
	}

	printf("\nNote regex for types are:\n");
	printf("  False - %s\n", regexFalse);
	printf("  True - %s\n", regexTrue);
	printf("  Int - %s\n", regexInt);
	printf("  Float - %s\n", regexFloat);
	printf("  String - %s\n", regexString);
	printf("  File - else\n");
}

void CommandLine::ArgLog(Arg& arg) {
	switch (arg.mType) {
	case Arg::INT: {
		printf("Type    : Int\n");
		printf("Range   : %lli - %lli\n", arg.mInt.mAcceptingRange.mBegin, arg.mInt.mAcceptingRange.mEnd);
		if (arg.mOptional) {
			printf("Default : %lli\n", arg.mInt.mDefault);
		}
		printf("Given   : %lli\n", arg.mInt.mVal);
	} break;
	case Arg::FLOAT: {
		printf("Type    : Float\n");
		printf("Range   : %f - %f\n", (halnf) arg.mFloat.mAcceptingRange.mBegin, (halnf) arg.mFloat.mAcceptingRange.mEnd);
		if (arg.mOptional) {
			printf("Default : %f\n", arg.mFloat.mDefault);
		}
		printf("Given   : %f\n", arg.mFloat.mVal);
	} break;
	case Arg::BOOL: {
		printf("Type    : Bool\n");
		if (arg.mOptional) {
			printf("Default : %s\n", arg.mBool.mDefault ? "True" : "False");
		}
		printf("Given   : %s\n", arg.mBool.mFlag ? "True" : "False");
	} break;
	case Arg::STR: {
		printf("Type    : String\n");
		if (arg.mOptional) {
			printf("Default : %s\n", arg.mStr.mDefault.read());
		}
		printf("Given   : %s\n", arg.mStr.mStr.read());
	} break;
	case Arg::FILE_IN: {
		printf("Type : File Input\n");
		printf("Given Path : %s\n", arg.mFile.mFilepath.read());
	} break;
	}
}

void CommandLine::initDefault(Arg& arg) {
	switch (arg.mType) {
		case Arg::INT: arg.mInt.mVal = arg.mInt.mDefault; break;
		case Arg::FLOAT: arg.mFloat.mVal = arg.mFloat.mDefault; break;
		case Arg::BOOL: arg.mBool.mFlag = arg.mBool.mDefault; break;
		case Arg::STR: arg.mStr.mStr = arg.mStr.mDefault; break;
		default:
			break;
	}
}

void CommandLine::parseArg(Arg& arg, const char* src) {
	mTokenizer.reset();
	mTokenizer.bindSource(src);

	auto tok = mTokenizer.readTok();
	if (tok < TokType::INT || tok > TokType::STR) {
		ErrInvalidArgSyntax(&arg);
	}

	auto val = mTokenizer.extractVal();

	switch (arg.mType) {
		case Arg::INT: {
			if (tok != TokType::INT) {
				ErrInvalidArgType(&arg);
				return;
			}
			auto converted = String::Logic::convertStringToValue(val.read(), arg.mInt.mVal);
			if (converted && arg.mInt.mVal < arg.mInt.mAcceptingRange.mBegin || arg.mInt.mVal > arg.mInt.mAcceptingRange.mEnd) {
				ErrValNotinRange(&arg);
				return;
			}
		} break;

		case Arg::FLOAT: {
			if (tok != TokType::FLOAT) {
				ErrInvalidArgType(&arg);
				return;
			}
			auto converted = String::Logic::convertStringToValue(val.read(), arg.mFloat.mVal);
			if (converted && arg.mFloat.mVal < arg.mFloat.mAcceptingRange.mBegin || arg.mFloat.mVal > arg.mFloat.mAcceptingRange.mEnd) {
				ErrValNotinRange(&arg);
				return;
			}
		} break;

		case Arg::STR: {
			if (tok != TokType::STR) {
				ErrInvalidArgType(&arg);
				return;
			}
			arg.mStr.mStr = val;
		} break;

		case Arg::BOOL: {
			if (tok != TokType::BOOL_FALSE && tok != TokType::BOOL_TRUE) {
				ErrInvalidArgType(&arg);
				return;
			}
			arg.mBool.mFlag = bool(val);
		} break;

		case Arg::FILE_IN: {
			if (tok != TokType::STR) {
				ErrInvalidArgType(&arg);
				return;
			}

			arg.mFile.mFilepath = val;

			/*
			// TODO : replace
			if (!File::exists(arg.mFile.mFilepath.cstr())) {
				ErrFileNotExists(&arg);
				return;
			}

			arg.mFile.mFile.open(arg.mFile.mFilepath.cstr(), osfile_openflags::LOAD);
			if (!arg.mFile.mFile.opened) {
				ErrFileCouldNotOpen(&arg);
				return;
			}
			*/

		} break;
	}

	arg.mIsPassed = true;
}
