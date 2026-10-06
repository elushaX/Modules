
#include "Callstack.hpp"

#include <iostream>

using namespace tp;

// frame to reference on no sapce left
static void errorNoSpaceLeft() {
	// to disable optimization of function
	static int dummy;
	dummy = 10;
}

ualni CallStackCapture::CallStack::getDepth() const {
	ualni len = 0;
	for (long long frame : frames) {
		if (!frame) {
			break;
		}
		len++;
	}
	return len++;
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
	for (ualni i = 0; i < MAX_CALL_DEPTH_CAPTURE; i++) {
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
	static_assert(MAX_CALL_CAPTURES_MEM_SIZE_MB * 1024 * 1024 > sizeof(CallStack));

	mBuffLoad = 0;
	mBuffLen = STACKS_LENGTH;
	mBuff = (CallStack*) malloc(mBuffLen * sizeof(CallStack));
	
	mErrorSnapshot.frames[0] = (alni) & errorNoSpaceLeft;


	platformInit();

	initialized = true;
}

const CallStackCapture::CallStack* CallStackCapture::getSnapshot() {

	if (mBuffLoad > mBuffLen) {
		return &mErrorSnapshot;
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

CallStackCapture::~CallStackCapture() { 
	free(mBuff); 
	initialized = false;
}

// ---------------------------------- Platform Depended ---------------------------------- //

#if defined(ENV_OS_LINUX)

#include <cstring>
#include <cxxabi.h>
#include <execinfo.h>
#include <malloc.h>

void CallStackCapture::platformInit() {
}

void CallStackCapture::platformDeinit() {
}

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
			std::strcpy(file, buff + ((sourceLen > CallStackCapture::MAX_DEBUG_INFO_LEN) ? (sourceLen - CallStackCapture::MAX_DEBUG_INFO_LEN) : 0));
			*line = strtoul(linePtr + 1, nullptr, 10);
			return;
		}
	}

	std::strcpy(file, "unresolved");
	*line = 0;
}

static void getDemangledName(const char* func, char* out) {
	int status;
	size_t funcDemangledSize = CallStackCapture::MAX_DEBUG_INFO_LEN;
	char* funcDemangled = (char*) malloc(funcDemangledSize);
	char* ret = abi::__cxa_demangle(func, funcDemangled, &funcDemangledSize, &status);
	if (status == 0) {
		funcDemangled = ret;
		auto funcLen = std::strlen(funcDemangled);
		std::strcpy(out, funcDemangled + ((funcLen > CallStackCapture::MAX_DEBUG_INFO_LEN) ? (funcLen - CallStackCapture::MAX_DEBUG_INFO_LEN) : 0));
		free(ret);
		return;
	}
	auto funcLen = std::strlen(func);
	std::strcpy(out, func + ((funcLen > CallStackCapture::MAX_DEBUG_INFO_LEN) ? (funcLen - CallStackCapture::MAX_DEBUG_INFO_LEN) : 0));
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
	if (snapshot) {
		for (auto frame : *snapshot) {
			auto symbols = getSymbols(frame.getFrame());
			printf("  %s   -----   %s:%llu\n", symbols->getFunc(), symbols->getFile(), symbols->getLine());
		}
	}
	printf("\n");
}

void CallStackCapture::logAll() {
	for (auto cs : *this) {
		printSnapshot(cs.getCallStack());
	}
}

#else
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

#pragma comment(lib, "dbghelp.lib")

void CallStackCapture::platformInit() {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
}

void CallStackCapture::platformDeinit() { 
	HANDLE process = GetCurrentProcess();
	SymCleanup(process);
}

void CallStackCapture::platformWriteStackTrace(CallStack* stack) {
	DWORD hash;
	CONTEXT context;
	memset(&context, 0, sizeof(CONTEXT));
	context.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&context);

	STACKFRAME64 stackFrame;
	memset(&stackFrame, 0, sizeof(STACKFRAME64));
#ifdef _M_IX86
	hash = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Ebp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Esp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
	hash = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Rbp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Rsp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

	for (int frameIndex = 0; frameIndex < MAX_CALL_DEPTH_CAPTURE; ++frameIndex) {
		if (!StackWalk64(
					hash,
					GetCurrentProcess(),
					GetCurrentThread(),
					&stackFrame,
					&context,
					NULL,
					SymFunctionTableAccess64,
					SymGetModuleBase64,
					NULL
				)) {

			stack->frames[frameIndex] = 0;
			break;
		}
		stack->frames[frameIndex] = (alni) (void*) stackFrame.AddrPC.Offset;
	}
	stack->frames[MAX_CALL_DEPTH_CAPTURE - 1] = 0;
}

void CallStackCapture::platformWriteDebugSymbols(FramePointer frame, DebugSymbols* out) {
	HANDLE process = GetCurrentProcess();

	IMAGEHLP_LINE64 lineInfo;
	DWORD displacement;
	lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	if (SymGetLineFromAddr64(process, (DWORD64) frame, &displacement, &lineInfo)) {
		strcpy(out->file, lineInfo.FileName);
		out->line = lineInfo.LineNumber;
	} else {
		strcpy(out->file, "unresolved");
		out->line = 0;
	}

	DWORD64 displacementSym = 0;
	char symbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_PATH] = { 0 };
	IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*) symbolBuffer;
	symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	symbol->MaxNameLength = MAX_PATH;

	if (SymGetSymFromAddr64(process, (DWORD64) frame, &displacementSym, symbol)) {
		strcpy(out->function, symbol->Name);
	} else {
		strcpy(out->function, "unresolved");
	}
}

void CallStackCapture::printSnapshot(const CallStack* snapshot) {
	printf("CallStack: \n");
	if (snapshot) {
		for (int i = 0; i < snapshot->getDepth(); ++i) {
			DebugSymbols symbols;
			platformWriteDebugSymbols(snapshot->frames[i], &symbols);
			printf("  %s   -----   %s:%lu\n", symbols.function, symbols.file, symbols.line);
		}
	}
	printf("\n");
}

void CallStackCapture::logAll() {
	for (auto cs : *this) {
		printSnapshot(cs.getCallStack());
	}
}
#endif