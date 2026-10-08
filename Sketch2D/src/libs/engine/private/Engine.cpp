#include "Engine.hpp"
#include "ThreadPlacement.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <sstream>

namespace tp::sketch2d {

	void Operation::finish(Layer& layer, const TileWork& work, const std::shared_ptr<Tile>& result) {
		result->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
		layer.noteChanged(work.coord);
	}

	Engine::Engine(size_t threads) { setThreads(threads); }

	Engine::~Engine() {
		drain();
		stopThreads();
	}

	ualni Engine::keyOf(const Layer* layer, const TileCoord& coord) {
		ualni hash = (ualni) (uintptr_t) layer * 0x9E3779B97F4A7C15ull;
		return hash ^ (coord.key() + 0x7F4A7C159E3779B9ull + (hash << 6) + (hash >> 2));
	}

	void Engine::stopThreads() {
		{
			std::lock_guard lock(mQueueMutex);
			mStopping = true;
		}
		mWake.notify_all();
		for (std::thread& thread : mThreads)
			thread.join();
		mThreads.clear();
		std::lock_guard lock(mQueueMutex);
		mStopping = false;
	}

	void Engine::setThreads(size_t threads) {
		drain();
		stopThreads();
		for (size_t index = 0; index < threads; index++)
			mThreads.emplace_back([this]() {
				ThreadPlacement::placeCallingThreadAsWorker();
				worker();
			});
	}

	void Engine::submit(Job job) {
		const int4 level = std::clamp(job.work.level, 0, LayerLevels::kLevels - 1);
		{
			std::lock_guard lock(mQueueMutex);
			const ualni key = keyOf(job.layer, job.work.coord);
			auto found = mQueued.presents(key);
			if (found.isValid()) mQueued.getSlotVal(found)++;
			else mQueued.put(key, 1);
			mQueues[level].push_back(std::move(job));
		}
		mWake.notify_one();
	}

	bool Engine::takeOne(Job& job) {
		for (int4 level = LayerLevels::kLevels - 1; level >= 0; level--) {
			std::deque<Job>& queue = mQueues[level];
			for (auto it = queue.begin(); it != queue.end(); ++it) {
				const ualni key = keyOf(it->layer, it->work.coord);
				if (mBusy.presents(key).isValid()) continue;
				job = std::move(*it);
				queue.erase(it);
				mBusy.put(key, true);
				mActive++;
				mRunning.push_back({ job.layer, job.work });
				return true;
			}
		}
		return false;
	}

	void Engine::runJob(Job& job) {
		if (!job.tile) job.tile = std::make_shared<Tile>(Tile::Uninitialized{});
		job.operation->run(job.work, job.tile);
		const ualni key = keyOf(job.layer, job.work.coord);
		{
			std::lock_guard lock(mDoneMutex);
			mDone.push_back(std::move(job));
		}
		mCompleted.fetch_add(1, std::memory_order_acq_rel);
		{
			std::lock_guard lock(mQueueMutex);
			mBusy.remove(key);
			for (size_t index = 0; index < mRunning.size(); index++) {
				if (keyOf(mRunning[index].first, mRunning[index].second.coord) != key) continue;
				mRunning[index] = mRunning.back();
				mRunning.pop_back();
				break;
			}
			mActive--;
		}
		mIdle.notify_all();
		mWake.notify_all();
	}

	void Engine::worker() {
		while (true) {
			Job job;
			{
				std::unique_lock lock(mQueueMutex);
				mWake.wait(lock, [&]() { return mStopping || takeOne(job); });
				if (!job.operation) return;
			}
			runJob(job);
		}
	}

	size_t Engine::runFor(double milliseconds) {
		if (isPausedHere()) return 0;
		const auto start = std::chrono::steady_clock::now();
		size_t ran = 0;
		while (true) {
			Job job;
			{
				std::lock_guard lock(mQueueMutex);
				if (!takeOne(job)) break;
			}
			runJob(job);
			ran++;
			const double elapsed =
				std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			if (elapsed >= milliseconds) break;
		}
		applyFor(milliseconds);
		return ran;
	}

	void Engine::drain() {
		if (isPausedHere()) return;
		if (mThreads.empty()) {
			while (true) {
				Job job;
				{
					std::lock_guard lock(mQueueMutex);
					if (!takeOne(job)) break;
				}
				runJob(job);
			}
		} else {
			std::unique_lock lock(mQueueMutex);
			mIdle.wait(lock, [this]() {
				if (mActive > 0) return false;
				for (const std::deque<Job>& queue : mQueues)
					if (!queue.empty()) return false;
				return true;
			});
		}
		apply();
	}

	void Engine::apply() { applyFor(std::numeric_limits<double>::infinity()); }

	void Engine::applyFor(double milliseconds) {
		const auto start = std::chrono::steady_clock::now();
		while (true) {
			Job job;
			{
				std::lock_guard lock(mDoneMutex);
				if (mDone.empty()) return;
				job = std::move(mDone.front());
				mDone.pop_front();
			}
			job.operation->finish(*job.layer, job.work, job.tile);
			{
				std::lock_guard lock(mQueueMutex);
				const ualni key = keyOf(job.layer, job.work.coord);
				auto found = mQueued.presents(key);
				if (found.isValid() && --mQueued.getSlotVal(found) <= 0) mQueued.remove(key);
			}
			const double spent = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			if (spent >= milliseconds) return;
		}
	}

	size_t Engine::pending() const {
		size_t total = 0;
		{
			std::lock_guard lock(mDoneMutex);
			total += mDone.size();
		}
		std::lock_guard lock(mQueueMutex);
		total += mActive;
		for (const std::deque<Job>& queue : mQueues)
			total += queue.size();
		return total;
	}

	size_t Engine::pending(int4 level) const {
		std::lock_guard lock(mQueueMutex);
		return mQueues[std::clamp(level, 0, LayerLevels::kLevels - 1)].size();
	}

	bool Engine::hasWork(const Layer& layer, const TileCoord& coord) const {
		std::lock_guard lock(mQueueMutex);
		return mQueued.presents(keyOf(&layer, coord)).isValid();
	}

	bool Engine::hasWorkFor(const Layer& layer) const {
		std::lock_guard lock(mQueueMutex);
		for (const std::deque<Job>& queue : mQueues)
			for (const Job& job : queue)
				if (job.layer == &layer) return true;
		for (const auto& running : mRunning)
			if (running.first == &layer) return true;
		return false;
	}

	std::string Engine::describeState() const {
		std::ostringstream text;
		text << "threads " << mThreads.size() << "\n";
		text << "completed " << completed() << "\n";
		text << "paused by a thread " << (mPausedBy.load() != std::thread::id() ? "yes" : "no") << "\n";

		std::unique_lock queue(mQueueMutex, std::try_to_lock);
		if (!queue.owns_lock()) text << "queue lock HELD by another thread\n";
		else {
			text << "running " << mActive << "\n";
			for (size_t level = 0; level < (size_t) LayerLevels::kLevels; level++)
				if (!mQueues[level].empty()) text << "queued at level " << level << ": " << mQueues[level].size() << "\n";
			for (const auto& running : mRunning)
				text << "running tile " << running.second.coord.x << "," << running.second.coord.y << " level "
						 << running.second.level << "\n";
			queue.unlock();
		}

		std::unique_lock done(mDoneMutex, std::try_to_lock);
		if (!done.owns_lock()) text << "done lock HELD by another thread\n";
		else text << "finished, waiting to apply " << mDone.size() << "\n";
		return text.str();
	}
}
