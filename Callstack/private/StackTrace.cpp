#include "StackTrace.hpp"

#include <algorithm>
#include <sstream>

#if defined(__linux__) || defined(__ANDROID__)

#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cxxabi.h>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <unwind.h>

namespace tp {

	namespace {
		struct UnwindState {
			uintptr_t* frames;
			size_t count;
			size_t capacity;
		};

		_Unwind_Reason_Code collectFrame(_Unwind_Context* context, void* argument) {
			auto* state = static_cast<UnwindState*>(argument);
			const uintptr_t address = (uintptr_t) _Unwind_GetIP(context);
			if (address == 0) return _URC_NO_REASON;
			if (state->count >= state->capacity) return _URC_END_OF_STACK;
			state->frames[state->count++] = address;
			return _URC_NO_REASON;
		}

		struct SampleSlot {
			std::atomic<int> tid{ 0 };
			std::atomic<size_t> depth{ 0 };
			std::atomic<bool> answered{ false };
			uintptr_t frames[StackTrace::kMaxFrames] = {};
		};

		constexpr size_t kMaxSampledThreads = 256;

		struct SampleBoard {
			SampleSlot slots[kMaxSampledThreads];
			std::atomic<size_t> used{ 0 };
		};

		SampleBoard& sampleBoard() {
			static SampleBoard shared;
			return shared;
		}

		std::mutex& samplingMutex() {
			static std::mutex shared;
			return shared;
		}

		int sampleSignal() { return SIGRTMIN + 3; }

		int currentTid() { return (int) syscall(SYS_gettid); }

		SampleSlot* slotOf(int tid) {
			SampleBoard& board = sampleBoard();
			const size_t used = std::min(board.used.load(std::memory_order_acquire), kMaxSampledThreads);
			for (size_t index = 0; index < used; index++)
				if (board.slots[index].tid.load(std::memory_order_relaxed) == tid) return &board.slots[index];
			return nullptr;
		}

		void answerSample(int, siginfo_t*, void*) {
			const int savedErrno = errno;
			if (SampleSlot* slot = slotOf(currentTid())) {
				slot->depth.store(
					StackTrace::captureCallingThread(slot->frames, StackTrace::kMaxFrames), std::memory_order_relaxed
				);
				slot->answered.store(true, std::memory_order_release);
			}
			errno = savedErrno;
		}

		std::string firstLine(const std::filesystem::path& path) {
			std::ifstream file(path);
			std::string line;
			std::getline(file, line);
			return line;
		}

		std::string schedulerState(const std::filesystem::path& task) {
			std::ifstream file(task / "status");
			std::string line;
			while (std::getline(file, line))
				if (line.rfind("State:", 0) == 0) return line.substr(line.find_first_not_of(" \t", 6));
			return "unknown";
		}

		std::vector<int> processThreadIds() {
			std::vector<int> tids;
			std::error_code error;
			for (const auto& entry : std::filesystem::directory_iterator("/proc/self/task", error)) {
				const std::string name = entry.path().filename().string();
				if (!name.empty() && std::all_of(name.begin(), name.end(), [](char c) {
							return std::isdigit((unsigned char) c);
						}))
					tids.push_back(std::stoi(name));
			}
			std::sort(tids.begin(), tids.end());
			return tids;
		}

		void armSlots(const std::vector<int>& tids) {
			SampleBoard& board = sampleBoard();
			for (size_t index = 0; index < tids.size(); index++) {
				SampleSlot& slot = board.slots[index];
				slot.answered.store(false, std::memory_order_relaxed);
				slot.depth.store(0, std::memory_order_relaxed);
				slot.tid.store(tids[index], std::memory_order_relaxed);
			}
			board.used.store(tids.size(), std::memory_order_release);
		}

		void waitForAnswers(size_t count, double timeoutMs) {
			SampleBoard& board = sampleBoard();
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double, std::milli>(timeoutMs);
			const auto allAnswered = [&]() {
				for (size_t index = 0; index < count; index++)
					if (!board.slots[index].answered.load(std::memory_order_acquire)) return false;
				return true;
			};
			while (!allAnswered() && std::chrono::steady_clock::now() < deadline)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		ThreadSample collect(const SampleSlot& slot, int tid) {
			const std::filesystem::path task = "/proc/self/task/" + std::to_string(tid);
			ThreadSample sample;
			sample.tid = tid;
			sample.name = firstLine(task / "comm");
			sample.state = schedulerState(task);
			sample.waitChannel = firstLine(task / "wchan");
			sample.answered = slot.answered.load(std::memory_order_acquire);
			if (sample.answered)
				sample.frames.assign(slot.frames, slot.frames + std::min(slot.depth.load(), StackTrace::kMaxFrames));
			return sample;
		}
	}

	size_t StackTrace::captureCallingThread(uintptr_t* frames, size_t capacity) {
		UnwindState state{ frames, 0, capacity };
		_Unwind_Backtrace(collectFrame, &state);
		return state.count;
	}

	FrameSymbol StackTrace::symbolize(uintptr_t address) {
		FrameSymbol symbol;
		Dl_info info{};
		if (!dladdr((void*) (address - 1), &info)) return symbol;
		if (info.dli_fname) symbol.module = std::filesystem::path(info.dli_fname).filename().string();
		symbol.moduleOffset = address - (uintptr_t) info.dli_fbase;
		if (!info.dli_sname) return symbol;
		int status = 0;
		char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
		symbol.function = status == 0 && demangled ? demangled : info.dli_sname;
		std::free(demangled);
		return symbol;
	}

	bool StackTrace::canSampleThreads() { return true; }

	bool StackTrace::isDebuggerAttached() {
		std::ifstream status("/proc/self/status");
		std::string line;
		while (std::getline(status, line))
			if (line.rfind("TracerPid:", 0) == 0) return std::strtol(line.c_str() + 10, nullptr, 10) != 0;
		return false;
	}

	void StackTrace::prepareThreadSampling() {
		static std::once_flag prepared;
		std::call_once(prepared, []() {
			struct sigaction action{};
			action.sa_sigaction = answerSample;
			action.sa_flags = SA_SIGINFO | SA_RESTART;
			sigemptyset(&action.sa_mask);
			sigaction(sampleSignal(), &action, nullptr);
			uintptr_t warmup[4];
			captureCallingThread(warmup, 4);
		});
	}

	std::vector<ThreadSample> StackTrace::sampleAllThreads(double answerTimeoutMs) {
		prepareThreadSampling();
		std::lock_guard lock(samplingMutex());
		std::vector<int> tids = processThreadIds();
		tids.erase(std::remove(tids.begin(), tids.end(), currentTid()), tids.end());
		if (tids.size() > kMaxSampledThreads) tids.resize(kMaxSampledThreads);

		armSlots(tids);
		for (int tid : tids)
			syscall(SYS_tgkill, getpid(), tid, sampleSignal());
		waitForAnswers(tids.size(), answerTimeoutMs);

		std::vector<ThreadSample> samples;
		for (size_t index = 0; index < tids.size(); index++)
			samples.push_back(collect(sampleBoard().slots[index], tids[index]));
		sampleBoard().used.store(0, std::memory_order_release);
		return samples;
	}

	int StackTrace::callingThreadId() { return currentTid(); }
}

#elif defined(_WIN32)

#include <windows.h>

namespace tp {

	size_t StackTrace::captureCallingThread(uintptr_t* frames, size_t capacity) {
		return (size_t) CaptureStackBackTrace(1, (DWORD) std::min<size_t>(capacity, 62), (void**) frames, nullptr);
	}

	FrameSymbol StackTrace::symbolize(uintptr_t address) {
		FrameSymbol symbol;
		HMODULE module = nullptr;
		if (!GetModuleHandleExA(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					(LPCSTR) address,
					&module
				))
			return symbol;
		char path[MAX_PATH] = {};
		GetModuleFileNameA(module, path, MAX_PATH);
		const std::string full = path;
		symbol.module = full.substr(full.find_last_of("\\/") + 1);
		symbol.moduleOffset = address - (uintptr_t) module;
		return symbol;
	}

	bool StackTrace::canSampleThreads() { return false; }
	bool StackTrace::isDebuggerAttached() { return IsDebuggerPresent() != 0; }
	void StackTrace::prepareThreadSampling() {}
	std::vector<ThreadSample> StackTrace::sampleAllThreads(double) { return {}; }
	int StackTrace::callingThreadId() { return (int) GetCurrentThreadId(); }
}

#endif

namespace tp {

	std::string StackTrace::describe(uintptr_t address) {
		const FrameSymbol symbol = symbolize(address);
		std::ostringstream text;
		text << "0x" << std::hex << address;
		if (!symbol.module.empty()) text << "  " << symbol.module << "+0x" << symbol.moduleOffset;
		text << std::dec;
		if (!symbol.function.empty()) text << "  " << symbol.function;
		if (!symbol.file.empty()) text << "  " << symbol.file << ":" << symbol.line;
		return text.str();
	}
}
