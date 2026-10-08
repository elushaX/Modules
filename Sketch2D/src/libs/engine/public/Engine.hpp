#pragma once

#include "Layer.hpp"
#include "LayerLevels.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tp::sketch2d {

	struct TileWork {
		TileCoord coord;
		int4 level = 0;
	};

	class Operation {
	public:
		virtual ~Operation() = default;

		[[nodiscard]] virtual const char* name() const = 0;
		virtual void run(const TileWork& work, Tile& target) = 0;
		virtual void run(const TileWork& work, const std::shared_ptr<Tile>& target) { run(work, *target); }
		virtual void finish(Layer& layer, const TileWork& work, const std::shared_ptr<Tile>& result);
	};

	class Engine {
	public:
		struct Job {
			std::shared_ptr<Operation> operation;
			std::shared_ptr<Tile> tile;
			Layer* layer = nullptr;
			TileWork work;
		};

		static constexpr double kFrameApplyMs = 2.0;

		explicit Engine(size_t threads = 0);
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		void setThreads(size_t threads);
		[[nodiscard]] size_t threads() const { return mThreads.size(); }

		void submit(Job job);
		void drain();
		size_t runFor(double milliseconds);
		void apply();
		void applyFor(double milliseconds);

		[[nodiscard]] size_t pending() const;
		[[nodiscard]] size_t pending(int4 level) const;
		[[nodiscard]] bool hasWork(const Layer& layer, const TileCoord& coord) const;
		[[nodiscard]] bool hasWorkFor(const Layer& layer) const;

		template <typename Fn>
		void forEachPending(Fn&& fn) const {
			std::lock_guard lock(mQueueMutex);
			for (const std::deque<Job>& queue : mQueues)
				for (const Job& job : queue)
					fn(*job.layer, job.work, false);
			for (const auto& running : mRunning)
				fn(*running.first, running.second, true);
		}
		[[nodiscard]] ualni completed() const { return mCompleted.load(std::memory_order_acquire); }
		[[nodiscard]] std::string describeState() const;
		[[nodiscard]] bool isPausedHere() const { return mPausedBy.load() == std::this_thread::get_id(); }

		template <typename Fn>
		void whilePaused(Fn&& fn) {
			if (isPausedHere()) {
				fn();
				return;
			}
			mPausedBy.store(std::this_thread::get_id());
			applyFor(kFrameApplyMs);
			fn();
			mPausedBy.store(std::thread::id());
		}

	private:
		[[nodiscard]] static ualni keyOf(const Layer* layer, const TileCoord& coord);
		bool takeOne(Job& job);
		void runJob(Job& job);
		void worker();
		void stopThreads();

		std::deque<Job> mQueues[LayerLevels::kLevels];
		Map<ualni, int4> mQueued;
		Map<ualni, bool> mBusy;
		mutable std::mutex mQueueMutex;
		std::condition_variable mWake;
		std::condition_variable mIdle;
		size_t mActive = 0;
		std::vector<std::pair<const Layer*, TileWork>> mRunning;
		bool mStopping = false;

		mutable std::mutex mDoneMutex;
		std::deque<Job> mDone;
		std::vector<std::thread> mThreads;
		std::atomic<ualni> mCompleted{ 0 };
		std::atomic<std::thread::id> mPausedBy{};
	};
}
