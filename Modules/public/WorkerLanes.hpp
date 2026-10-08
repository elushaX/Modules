#pragma once

#include "ThreadPlacement.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace tp {

	template <typename Job>
	class WorkerLanes {
		struct Lane {
			std::mutex queueMutex;
			std::condition_variable wake;
			std::vector<Job> queue;
			bool stopping = false;
			std::mutex workMutex;
			std::thread thread;
		};

	public:
		using Handler = std::function<void(size_t lane, Job& job)>;

		class Pause {
		public:
			explicit Pause(WorkerLanes* owner) :
				mOwner(owner) {
				if (!mOwner) return;
				mOwner->mPaused.store(true, std::memory_order_release);
				for (auto& lane : mOwner->mLanes)
					lane->workMutex.lock();
			}

			~Pause() { release(); }

			Pause(const Pause&) = delete;
			Pause& operator=(const Pause&) = delete;
			Pause(Pause&& other) noexcept :
				mOwner(other.mOwner) {
				other.mOwner = nullptr;
			}

			void release() {
				if (!mOwner) return;
				for (auto& lane : mOwner->mLanes)
					lane->workMutex.unlock();
				{
					std::lock_guard lock(mOwner->mGateMutex);
					mOwner->mPaused.store(false, std::memory_order_release);
				}
				mOwner->mGate.notify_all();
				mOwner = nullptr;
			}

		private:
			WorkerLanes* mOwner;
		};

		WorkerLanes(size_t lanes, Handler handler) :
			mHandler(std::move(handler)) {
			for (size_t index = 0; index < lanes; index++)
				mLanes.push_back(std::make_unique<Lane>());
			for (size_t index = 0; index < mLanes.size(); index++)
				mLanes[index]->thread = std::thread([this, index]() { run(index); });
		}

		~WorkerLanes() {
			for (auto& lane : mLanes) {
				{
					std::lock_guard lock(lane->queueMutex);
					lane->stopping = true;
				}
				lane->wake.notify_one();
			}
			for (auto& lane : mLanes)
				lane->thread.join();
		}

		WorkerLanes(const WorkerLanes&) = delete;
		WorkerLanes& operator=(const WorkerLanes&) = delete;

		[[nodiscard]] size_t laneCount() const { return mLanes.size(); }
		[[nodiscard]] size_t pending() const { return mPending.load(std::memory_order_acquire); }

		void push(size_t lane, Job job) {
			if (mLanes.empty()) {
				mHandler(0, job);
				return;
			}

			Lane& target = *mLanes[lane % mLanes.size()];
			mPending.fetch_add(1, std::memory_order_relaxed);
			bool wasEmpty = false;
			{
				std::lock_guard lock(target.queueMutex);
				wasEmpty = target.queue.empty();
				target.queue.push_back(std::move(job));
			}
			if (wasEmpty) target.wake.notify_one();
		}

		void pushAll(size_t lane, std::vector<Job>& jobs) {
			if (jobs.empty()) return;
			if (mLanes.empty()) {
				for (Job& job : jobs)
					mHandler(0, job);
				jobs.clear();
				return;
			}

			Lane& target = *mLanes[lane % mLanes.size()];
			mPending.fetch_add(jobs.size(), std::memory_order_relaxed);
			bool wasEmpty = false;
			{
				std::lock_guard lock(target.queueMutex);
				wasEmpty = target.queue.empty();
				for (Job& job : jobs)
					target.queue.push_back(std::move(job));
			}
			jobs.clear();
			if (wasEmpty) target.wake.notify_one();
		}

		void wait() {
			if (pending() == 0) return;
			std::unique_lock lock(mIdleMutex);
			mIdle.wait(lock, [this]() { return pending() == 0; });
		}

		[[nodiscard]] Pause pause() { return Pause(mLanes.empty() ? nullptr : this); }

		void runQueuedHere() {
			for (size_t index = 0; index < mLanes.size(); index++) {
				Lane& lane = *mLanes[index];
				std::vector<Job> batch;
				{
					std::lock_guard lock(lane.queueMutex);
					batch.swap(lane.queue);
				}
				for (Job& job : batch) {
					mHandler(index, job);
					mPending.fetch_sub(1, std::memory_order_acq_rel);
				}
			}
		}

	private:
		void waitAtGate() {
			if (!mPaused.load(std::memory_order_acquire)) return;
			std::unique_lock lock(mGateMutex);
			mGate.wait(lock, [this]() { return !mPaused.load(std::memory_order_acquire); });
		}

		void run(size_t index) {
			ThreadPlacement::placeCallingThreadAsWorker();
			Lane& lane = *mLanes[index];
			std::vector<Job> batch;
			while (true) {
				{
					std::unique_lock lock(lane.queueMutex);
					lane.wake.wait(lock, [&lane]() { return lane.stopping || !lane.queue.empty(); });
					if (lane.queue.empty()) return;
					batch.swap(lane.queue);
				}

				for (size_t at = 0; at < batch.size(); at++) {
					if (mPaused.load(std::memory_order_acquire)) {
						{
							std::lock_guard lock(lane.queueMutex);
							lane.queue.insert(
								lane.queue.begin(),
								std::make_move_iterator(batch.begin() + (std::ptrdiff_t) at),
								std::make_move_iterator(batch.end())
							);
						}
						break;
					}
					{
						std::lock_guard work(lane.workMutex);
						if (mPaused.load(std::memory_order_acquire)) {
							std::lock_guard lock(lane.queueMutex);
							lane.queue.insert(
								lane.queue.begin(),
								std::make_move_iterator(batch.begin() + (std::ptrdiff_t) at),
								std::make_move_iterator(batch.end())
							);
							break;
						}
						mHandler(index, batch[at]);
					}
					if (mPending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
						std::lock_guard lock(mIdleMutex);
						mIdle.notify_all();
					}
				}
				batch.clear();
				waitAtGate();
			}
		}

		Handler mHandler;
		std::vector<std::unique_ptr<Lane>> mLanes;
		std::atomic<size_t> mPending{ 0 };
		std::mutex mIdleMutex;
		std::condition_variable mIdle;
		std::atomic<bool> mPaused{ false };
		std::mutex mGateMutex;
		std::condition_variable mGate;
	};
}
