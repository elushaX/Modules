#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tp {

	struct FrameSymbol {
		std::string module;
		uintptr_t moduleOffset = 0;
		std::string function;
		std::string file;
		size_t line = 0;
	};

	struct ThreadSample {
		int tid = 0;
		std::string name;
		std::string state;
		std::string waitChannel;
		bool answered = false;
		std::vector<uintptr_t> frames;
	};

	class StackTrace {
	public:
		static constexpr size_t kMaxFrames = 64;

		static size_t captureCallingThread(uintptr_t* frames, size_t capacity);
		[[nodiscard]] static FrameSymbol symbolize(uintptr_t address);
		[[nodiscard]] static std::string describe(uintptr_t address);

		[[nodiscard]] static bool canSampleThreads();
		[[nodiscard]] static bool isDebuggerAttached();
		static void prepareThreadSampling();
		[[nodiscard]] static std::vector<ThreadSample> sampleAllThreads(double answerTimeoutMs = 200);
		[[nodiscard]] static int callingThreadId();
	};
}
