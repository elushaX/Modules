#include "ParallelFor.hpp"

#include "UnitTest++/UnitTest++.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

using namespace tp;

SUITE(ParallelFor) {
	TEST(EveryIndexRunsOnce) {
		ParallelFor parallel(4);
		std::vector<std::atomic<int>> hits(1000);
		parallel.run(hits.size(), [&hits](size_t index) { hits[index]++; });
		for (const auto& hit : hits)
			CHECK_EQUAL(1, hit.load());
	}

	TEST(RunsAreRepeatableAndReturnWhenDone) {
		ParallelFor parallel(3);
		for (int round = 0; round < 200; round++) {
			std::atomic<int> sum{ 0 };
			parallel.run(37, [&sum](size_t index) { sum += (int) index; });
			CHECK_EQUAL(666, sum.load());
		}
	}

	TEST(WorkIsSharedWithHelperThreads) {
		ParallelFor parallel(4);
		CHECK_EQUAL(4u, parallel.threads());
		std::mutex mutex;
		std::set<std::thread::id> threads;
		parallel.run(64, [&](size_t) {
			std::this_thread::sleep_for(std::chrono::microseconds(500));
			std::lock_guard lock(mutex);
			threads.insert(std::this_thread::get_id());
		});
		CHECK(threads.size() > 1);
	}

	TEST(SingleThreadRunsInline) {
		ParallelFor parallel(1);
		const std::thread::id caller = std::this_thread::get_id();
		bool inline_ = true;
		parallel.run(10, [&](size_t) { inline_ = inline_ && std::this_thread::get_id() == caller; });
		CHECK(inline_);
		parallel.run(0, [](size_t) {});
	}
}
