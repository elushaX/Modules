#include "ThreadPlacement.hpp"

#include "UnitTest++/UnitTest++.h"

#include <algorithm>
#include <atomic>
#include <thread>

using namespace tp;

namespace {
	bool contains(const std::vector<int>& cpus, int cpu) { return std::count(cpus.begin(), cpus.end(), cpu) > 0; }
}

SUITE(ThreadPlacement) {
	TEST(InteractiveThreadOwnsItsWholePhysicalCore) {
		const ThreadPlacement::Layout layout = ThreadPlacement::layoutFor(2, { { 0, 1 }, { 2, 3 }, { 4, 5 } });
		CHECK(layout.interactive == std::vector<int>({ 2, 3 }));
		CHECK(layout.workers == std::vector<int>({ 0, 1, 4, 5 }));
	}

	TEST(WorkersNeverShareTheInteractiveCore) {
		const std::vector<std::vector<int>> groups = { { 0, 6 }, { 1, 7 }, { 2, 8 }, { 3, 9 }, { 4, 10 }, { 5, 11 } };
		const ThreadPlacement::Layout layout = ThreadPlacement::layoutFor(7, groups);
		CHECK(contains(layout.interactive, 1));
		CHECK(contains(layout.interactive, 7));
		for (int cpu : layout.workers) {
			CHECK(cpu != 1);
			CHECK(cpu != 7);
		}
		CHECK_EQUAL(10u, layout.workers.size());
	}

	TEST(SingleCoreKeepsWorkersOnIt) {
		const ThreadPlacement::Layout layout = ThreadPlacement::layoutFor(0, { { 0, 1 } });
		CHECK(layout.workers == std::vector<int>({ 0, 1 }));
	}

	TEST(WorkerThreadsLandOutsideTheReservedCore) {
		ThreadPlacement::reserveCallingThreadForInteraction();
		const ThreadPlacement::Layout layout = ThreadPlacement::current();
		if (layout.workers.empty() || layout.workers == layout.interactive) return;
		for (int cpu : ThreadPlacement::callingThreadCpus())
			CHECK(contains(layout.interactive, cpu));
		std::vector<int> seen;
		std::thread worker([&seen]() {
			ThreadPlacement::placeCallingThreadAsWorker();
			seen = ThreadPlacement::callingThreadCpus();
		});
		worker.join();
		REQUIRE CHECK(!seen.empty());
		for (int cpu : seen)
			CHECK(!contains(layout.interactive, cpu));
	}
}
