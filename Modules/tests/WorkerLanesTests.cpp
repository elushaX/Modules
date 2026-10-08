#include "WorkerLanes.hpp"

#include "UnitTest++/UnitTest++.h"

#include <chrono>
#include <set>
#include <thread>
#include <vector>

using namespace tp;

namespace {
	struct Step {
		size_t lane = 0;
		int value = 0;
	};
}

SUITE(WorkerLanes) {
	TEST(WaitReturnsAfterEveryJobRan) {
		std::atomic<int> sum{ 0 };
		WorkerLanes<Step> lanes(4, [&sum](size_t, Step& step) { sum += step.value; });
		for (int value = 1; value <= 1000; value++)
			lanes.push((size_t) value, { (size_t) value, value });
		lanes.wait();
		CHECK_EQUAL(500500, sum.load());
		CHECK_EQUAL(0u, lanes.pending());
	}

	TEST(OneLaneRunsItsJobsInOrder) {
		std::vector<int> seen[3];
		WorkerLanes<Step> lanes(3, [&seen](size_t, Step& step) { seen[step.lane].push_back(step.value); });
		for (int value = 0; value < 300; value++)
			lanes.push((size_t) value % 3, { (size_t) value % 3, value });
		lanes.wait();
		for (size_t lane = 0; lane < 3; lane++) {
			CHECK_EQUAL(100u, seen[lane].size());
			for (size_t index = 1; index < seen[lane].size(); index++)
				CHECK(seen[lane][index - 1] < seen[lane][index]);
		}
	}

	TEST(JobsRunOffTheCallingThread) {
		std::mutex mutex;
		std::set<std::thread::id> threads;
		WorkerLanes<Step> lanes(2, [&](size_t, Step&) {
			std::lock_guard lock(mutex);
			threads.insert(std::this_thread::get_id());
		});
		for (int value = 0; value < 64; value++)
			lanes.push((size_t) value, {});
		lanes.wait();
		CHECK(threads.count(std::this_thread::get_id()) == 0);
		CHECK(!threads.empty());
	}

	TEST(BatchedJobsKeepTheirOrder) {
		std::vector<int> seen;
		WorkerLanes<Step> lanes(2, [&seen](size_t, Step& step) { seen.push_back(step.value); });
		std::vector<Step> batch;
		for (int value = 0; value < 50; value++)
			batch.push_back({ 1, value });
		lanes.pushAll(1, batch);
		CHECK(batch.empty());
		lanes.wait();
		CHECK_EQUAL(50u, seen.size());
		for (size_t index = 0; index < seen.size(); index++)
			CHECK_EQUAL((int) index, seen[index]);
	}

	TEST(PauseHoldsJobsUntilReleased) {
		std::atomic<int> ran{ 0 };
		WorkerLanes<Step> lanes(2, [&ran](size_t, Step&) { ran++; });
		{
			auto pause = lanes.pause();
			for (int value = 0; value < 20; value++)
				lanes.push((size_t) value, {});
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			CHECK_EQUAL(0, ran.load());
			CHECK_EQUAL(20u, lanes.pending());
		}
		lanes.wait();
		CHECK_EQUAL(20, ran.load());
	}

	TEST(ZeroLanesRunJobsImmediately) {
		int sum = 0;
		WorkerLanes<Step> lanes(0, [&sum](size_t, Step& step) { sum += step.value; });
		lanes.push(0, { 0, 7 });
		CHECK_EQUAL(7, sum);
		CHECK_EQUAL(0u, lanes.pending());
		lanes.wait();
	}

	TEST(DestroyingFinishesQueuedJobs) {
		std::atomic<int> ran{ 0 };
		{
			WorkerLanes<Step> lanes(2, [&ran](size_t, Step&) { ran++; });
			for (int value = 0; value < 500; value++)
				lanes.push((size_t) value, {});
		}
		CHECK_EQUAL(500, ran.load());
	}
}
