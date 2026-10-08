#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace tp {

	class ParallelFor {
	public:
		explicit ParallelFor(size_t threads);
		~ParallelFor();

		ParallelFor(const ParallelFor&) = delete;
		ParallelFor& operator=(const ParallelFor&) = delete;

		[[nodiscard]] size_t threads() const { return mThreads.size() + 1; }

		void run(size_t count, const std::function<void(size_t index)>& body);

	private:
		void work(size_t generation);
		void drain();

		std::vector<std::thread> mThreads;
		std::mutex mMutex;
		std::condition_variable mStart;
		std::condition_variable mDone;
		const std::function<void(size_t)>* mBody = nullptr;
		size_t mCount = 0;
		std::atomic<size_t> mNext{ 0 };
		size_t mBusy = 0;
		size_t mGeneration = 0;
		bool mStopping = false;
	};
}
