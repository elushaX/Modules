
#include "Debugging.hpp"
#include "Utils.hpp"

#include <iostream>

using namespace tp;

CallStackCapture* tp::gCSCapture = nullptr;

void initializeCallStackCapture() { gCSCapture = new (malloc(sizeof(CallStackCapture))) CallStackCapture(); }

void deinitializeCallStackCapture() {
	gCSCapture->~CallStackCapture();
	free(gCSCapture);
}

ualni CallStackCapture::CallStack::getDepth() const {
	ualni len = 0;
	for (long long frame : frames) {
		if (!frame) {
			break;
		}
		len++;
	}
	ualni stripedLen = 0;
	for (auto i = FRAMES_TO_SKIP_START; i < len - FRAMES_TO_SKIP_END; i++) {
		if (!frames[i]) {
			break;
		}
		stripedLen++;
	}
	return stripedLen;
}

ualni CallStackCapture::hashCallStack(CallStackKey key) {
	auto const cs = key.cs;
	ualni out = 0;
	for (ualni i = 0; cs->frames[i]; i++) {
		out += cs->frames[i];
	}
	return out;
}

bool CallStackCapture::CallStackKey::operator==(const CallStackCapture::CallStackKey& in) const {
	for (auto i : Range(MAX_CALL_DEPTH_CAPTURE)) {
		if (cs->frames[i] != in.cs->frames[i]) {
			return false;
		}
		if (cs->frames[i] == 0 && in.cs->frames[i] == 0) {
			return true;
		}
	}
	DEBUG_ASSERT(0 && "Must Not Happen")
	return true;
}

CallStackCapture::CallStackCapture() {
	static_assert(MAX_CALL_DEPTH_CAPTURE >= 1);
	static_assert(MAX_CALL_CAPTURES_MEM_SIZE_MB > 0);
	static_assert(MAX_DEBUG_INFO_LEN > sizeof("unresolved"));
	static_assert(FRAMES_TO_SKIP_END >= 0 && FRAMES_TO_SKIP_START >= 0);
	static_assert(MAX_CALL_DEPTH_CAPTURE > FRAMES_TO_SKIP_START + FRAMES_TO_SKIP_END);
	static_assert(MAX_CALL_CAPTURES_MEM_SIZE_MB * 1024 * 1024 > sizeof(CallStack));

	MODULE_SANITY_CHECK(gModuleUtils)
	mBuffLoad = 0;
	mBuffLen = (MAX_CALL_CAPTURES_MEM_SIZE_MB * 1024 * 1024) / sizeof(CallStack);
	mBuff = (CallStack*) malloc(mBuffLen * sizeof(CallStack));
}

const CallStackCapture::CallStack* CallStackCapture::getSnapshot() {
	if (mBuffLoad > mBuffLen) {
		static CallStack cs;
		cs.frames[0] = 0;
		return &cs;
	}

	CallStack* cs = &mBuff[mBuffLoad];
	platformWriteStackTrace(cs);

	auto idx = mSnapshots.presents({ cs });
	if (idx) {
		return mSnapshots.getSlotVal(idx);
	}

	mSnapshots.put({ cs }, cs);

	mBuffLoad++;
	return cs;
}

const CallStackCapture::DebugSymbols* CallStackCapture::getSymbols(FramePointer frame) {
	auto idx = mSymbols.presents(frame);
	if (idx) {
		return &mSymbols.getSlotVal(idx);
	}

	mSymbols.put(frame, {});
	auto symbols = &mSymbols.get(frame);

	platformWriteDebugSymbols(frame, symbols);
	return symbols;
}

void CallStackCapture::clear() {
	mBuffLoad = 0;
	mSnapshots.removeAll();
	mSymbols.removeAll();
}

CallStackCapture::~CallStackCapture() { free(mBuff); }

// ---------------------------------- Platform Depended ---------------------------------- //

#if defined(ENV_OS_LINUX)

#include <cstring>
#include <cxxabi.h>
#include <execinfo.h>
#include <malloc.h>

void CallStackCapture::platformWriteStackTrace(CallStack* stack) {
	auto depth = backtrace((void**) stack->frames, (int) MAX_CALL_DEPTH_CAPTURE - 1);
	stack->frames[depth] = 0;
}

static void getGetSourceFromBinaryAddress(const char* binary, const char* address, char* file, ualni* line) {
	static char buff[1024];

	snprintf(buff, sizeof(buff), "addr2line -e %s -a %s", binary, address);
	FILE* pipe = popen(buff, "r");
	if (pipe) {
		fgets(buff, sizeof(buff), pipe);
		fgets(buff, sizeof(buff), pipe);
		pclose(pipe);
		char* linePtr = strchr(buff, ':');

		printf("%s\n", buff);

		if (linePtr != nullptr && buff[0] != '?' && buff[1] != '?' && buff[2] != ':') {
			*linePtr = '\0';
			auto sourceLen = std::strlen(buff);
			std::strcpy(file, buff + ((sourceLen > MAX_DEBUG_INFO_LEN) ? (sourceLen - MAX_DEBUG_INFO_LEN) : 0));
			*line = strtoul(linePtr + 1, nullptr, 10);
			return;
		}
	}

	std::strcpy(file, "unresolved");
	*line = 0;
}

static void getDemangledName(const char* func, char* out) {
	int status;
	size_t funcDemangledSize = MAX_DEBUG_INFO_LEN;
	char* funcDemangled = (char*) malloc(funcDemangledSize);
	char* ret = abi::__cxa_demangle(func, funcDemangled, &funcDemangledSize, &status);
	if (status == 0) {
		funcDemangled = ret;
		auto funcLen = std::strlen(funcDemangled);
		std::strcpy(out, funcDemangled + ((funcLen > MAX_DEBUG_INFO_LEN) ? (funcLen - MAX_DEBUG_INFO_LEN) : 0));
		free(ret);
		return;
	}
	auto funcLen = std::strlen(func);
	std::strcpy(out, func + ((funcLen > MAX_DEBUG_INFO_LEN) ? (funcLen - MAX_DEBUG_INFO_LEN) : 0));
	free(funcDemangled);
}

void CallStackCapture::platformWriteDebugSymbols(FramePointer frame, DebugSymbols* out) {
	void* addrList[1] = { (void*) frame };
	auto symbolsArray = backtrace_symbols(addrList, 1);

	// 'bin(fun+addr)'
	char* bin = *symbolsArray;
	char* func = nullptr;
	char* offset = nullptr;

	// 'bin fun+addr'
	for (char* p = bin; *p; ++p) {
		if (*p == '(') {
			*p = 0;
			func = p + 1;
		} else if (*p == '+') {
			offset = p;
		} else if (*p == ')' && offset) {
			*p = 0;
		}
	}

	if (func && offset) {
		getGetSourceFromBinaryAddress(bin, func, out->file, &out->line);

		if (offset != func) {
			*offset = 0;
			getDemangledName(func, out->function);
		} else {
			std::strcpy(out->function, "unresolved");
		}
	} else {
		std::strcpy(out->file, "unresolved");
		std::strcpy(out->function, "unresolved");
	}

	free(symbolsArray);
}

void CallStackCapture::printSnapshot(const CallStack* snapshot) {
	printf("CallStack: \n");
	for (auto frame : *snapshot) {
		auto symbols = gCSCapture->getSymbols(frame.getFrame());
		printf("  %s   -----   %s:%llu\n", symbols->getFunc(), symbols->getFile(), symbols->getLine());
	}
	printf("\n");
}

void CallStackCapture::logAll() {
	for (auto cs : *this) {
		printSnapshot(cs.getCallStack());
	}
}

#else
void CallStackCapture::platformWriteStackTrace(CallStack* stack) { stack->frames[0] = 0; }
void CallStackCapture::platformWriteDebugSymbols(FramePointer frame, DebugSymbols* out) {
	std::strcpy(out->file, "unresolved");
	std::strcpy(out->function, "unresolved");
}
#endif