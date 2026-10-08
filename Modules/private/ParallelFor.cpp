#include "ParallelFor.hpp"
#include "ThreadPlacement.hpp"

namespace tp {

	ParallelFor::ParallelFor(size_t threads) {
		for (size_t index = 1; index < threads; index++)
			mThreads.emplace_back([this]() {
				ThreadPlacement::placeCallingThreadAsWorker();
				work(0);
			});
	}

	ParallelFor::~ParallelFor() {
		{
			std::lock_guard lock(mMutex);
			mStopping = true;
		}
		mStart.notify_all();
		for (std::thread& thread : mThreads)
			thread.join();
	}

	void ParallelFor::drain() {
		while (true) {
			const size_t index = mNext.fetch_add(1, std::memory_order_relaxed);
			if (index >= mCount) return;
			(*mBody)(index);
		}
	}

	void ParallelFor::work(size_t seen) {
		while (true) {
			{
				std::unique_lock lock(mMutex);
				mStart.wait(lock, [&]() { return mStopping || (mGeneration != seen && mBody); });
				if (mStopping) return;
				seen = mGeneration;
				mBusy++;
			}
			drain();
			bool last = false;
			{
				std::lock_guard lock(mMutex);
				last = --mBusy == 0;
			}
			if (last) mDone.notify_one();
		}
	}

	void ParallelFor::run(size_t count, const std::function<void(size_t)>& body) {
		if (count == 0) return;
		if (mThreads.empty() || count == 1) {
			for (size_t index = 0; index < count; index++)
				body(index);
			return;
		}

		{
			std::lock_guard lock(mMutex);
			mBody = &body;
			mCount = count;
			mNext.store(0, std::memory_order_relaxed);
			mGeneration++;
		}
		mStart.notify_all();
		drain();

		std::unique_lock lock(mMutex);
		mDone.wait(lock, [this]() { return mBusy == 0; });
		mBody = nullptr;
		mCount = 0;
	}
}
