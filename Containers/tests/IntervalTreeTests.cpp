#include "IntervalTree.hpp"
#include "Testing.hpp"
#include "Tests.hpp"

#include "Buffer.hpp"

#include <cstdio>

using namespace tp;

struct Interval {

	[[nodiscard]] bool overlaps(const Interval& in) const { return in.start < end && in.end > start; }

	halnf start{};
	halnf end{};
	bool ignore = false;

	void random(halnf span, halnf scale = 1.f) {
		start = ((halnf) randomFloat()) * (span);
		end = ((halnf) randomFloat()) * (span);
		if (start > end) swap(start, end);

		auto len = (end - start) * scale * 0.5f;
		auto mid = (start + end) / 2.f;

		start = mid - len;
		end = mid + len;
	}

	void randomSized(halnf span, halnf size, halnf wobble) {
		start = ((halnf) randomFloat()) * (span);
		end = start + size + (halnf) (randomFloat() * wobble);
	}
};

TEST_DEF_STATIC(FunctionalitySimple) {

	IntervalTree<ualni, ualni> intervalTree;

	intervalTree.insert({ 0, 10 }, 1);
	intervalTree.insert({ 3, 6 }, 2);
	intervalTree.insert({ 8, 12 }, 3);

	intervalTree.forEachIntersection(4, 5, [](alni start, ualni end, ualni data) {
		printf("%i", int(data));
		printf("\n");
	});
}

TEST_DEF_STATIC(FunctionalityScale) {

	const int NUM_TEST_INTERVALS = 1000;
	const halnf SPAN = 100;

	Buffer<Interval> pool;
	IntervalTree<halnf, ualni> intervalTree;

	Buffer<Interval> testIntervals;

	auto test = [&]() {
		for (auto testInterval : testIntervals) {

			Buffer<ualni> correct;
			Buffer<ualni> result;

			ualni idx = 0;
			for (auto interval : pool) {
				if (!interval->ignore && interval->overlaps(testInterval.data())) {
					correct.append(idx);
				}
				idx++;
			}

			intervalTree.forEachIntersection(testInterval->start, testInterval->end, [&](alni start, ualni end, ualni data) {
				result.append(data);
			});

			TEST(correct.size() == result.size());

			if (!(correct.size() == result.size())) {
				printf("intersections - \n");

				for (auto i : correct) {
					printf(" %i", (int) i.data());
				}
				printf("\n");

				for (auto i : result) {
					printf(" %i", (int) i.data());
				}
				printf("\n\n");
			}
			// todo compare containers
		}
	};

	// initialize
	for (auto i : Range<ualni>(NUM_TEST_INTERVALS)) {
		auto interval = Interval();
		interval.random(SPAN);

		pool.append(interval);
		intervalTree.insert({ interval.start, interval.end }, i);

		interval.random(SPAN * 2.f, (halnf) randomFloat());

		testIntervals.append(interval);
	}

	test();

	// remove some
	for (auto i : Range<ualni>(NUM_TEST_INTERVALS / 2)) {
		auto idx = ualni(randomFloat() * (alnf) pool.size());
		pool[idx].ignore = true;
		intervalTree.remove({ pool[idx].start, pool[idx].end });
	}

	test();
}

TEST_DEF_STATIC(Efficency) {

	struct Stat {
		halnf numItems = 0;
		halnf avgFound = 0;
		halnf avgChecks = 0;
	};

	auto test = [&](ualni NUM_TEST_INTERVALS, ualni NUM_CHECKS) {
		const auto SPAN = (halnf) (halnf(NUM_TEST_INTERVALS));
		const auto SCALE = (halnf) (2.f);

		IntervalTree<halnf, ualni> intervalTree;
		Buffer<Interval> testIntervals;

		auto WOBBLE = SPAN * 0;
		for (auto i : Range<ualni>(NUM_TEST_INTERVALS)) {
			auto interval = Interval();
			interval.randomSized(SPAN, SCALE, WOBBLE);
			intervalTree.insert({ interval.start, interval.end }, i);
			// WOBBLE -= 1.f;
		}

		for (auto i : Range(0))
			intervalTree.insert({ (halnf) i * 0.01f, SPAN }, 0);

		WOBBLE = 0;
		for (auto i : Range<ualni>(NUM_CHECKS)) {
			auto interval = Interval();
			interval.randomSized(SPAN, SCALE, WOBBLE);
			testIntervals.append(interval);
		}

		ualni debugMaxChecks = 0;
		ualni debugMaxFound = 0;
		halnf debugAvgChecks = 0;
		halnf debugAvgFound = 0;

		for (auto testInterval : testIntervals) {
			ualni debugFound = 0;
			ualni debug = intervalTree.forEachIntersection(
				testInterval->start,
				testInterval->end,
				[&](ualni start, ualni end, ualni data) {
					debugFound++;
					//
				}
			);

			if (debug > debugMaxChecks) {
				debugMaxChecks = debug;
				debugMaxFound = debugFound;
			}

			debugMaxChecks = max(debug, debugMaxChecks);
			debugAvgChecks += (halnf) debug;
			debugAvgFound += (halnf) debugFound;
		}

		debugAvgChecks /= (halnf) testIntervals.size();
		debugAvgFound /= (halnf) testIntervals.size();

		printf("\nItems : %llu\n", NUM_TEST_INTERVALS);
		printf("Avg(checks) : %f\n", debugAvgChecks);
		printf("Avg(found) : %f\n", debugAvgFound);
		printf("Max checks : %llu\n", debugMaxChecks);
		printf("Max checks found : %llu\n", debugMaxFound);
		printf("N(Avg(checks) / Avg(N(items)) : %f\n", debugAvgChecks / (halnf) intervalTree.size());
		printf("N(Avg(found)) / Avg(N(items)) : %f\n", debugAvgFound / (halnf) intervalTree.size());
		printf("Avg(found) / Avg(checks) : %f\n", debugAvgFound / debugAvgChecks);

		return Stat{ (halnf) NUM_TEST_INTERVALS,
								 debugAvgFound / (halnf) intervalTree.size(),
								 debugAvgChecks / (halnf) intervalTree.size() };
	};

	Buffer<Stat> stats;
	for (auto i : Range(2, 5)) {
		Stat stat = test(pow(10, i), 100);
		stats.append(stat);
	}

	printf("\nChecks: ");
	for (auto stat : stats)
		printf("%e ", stat->avgChecks);

	printf("\nHits:   ");
	for (auto stat : stats)
		printf("%e ", stat->avgFound);

	printf("\nItems:  ");
	for (auto stat : stats)
		printf("%e ", stat->numItems);

	printf("\n\n");
}

TEST_DEF_STATIC(FunctionalityComplex) {
	IntervalTree<ualni, ualni> intervals;

	struct QueryResult {
		ualni numFound = 0;
		ualni lastDataFound = 0;

		[[nodiscard]] bool isSingleData(ualni aData) const { return numFound == 1 & lastDataFound == aData; }
		[[nodiscard]] bool notFound() const { return numFound == 0; }
		[[nodiscard]] bool found(ualni num) const { return numFound == num; }
	};

	auto makeQuery = [&](ualni aStart, ualni aEnd) {
		QueryResult out;
		intervals.forEachIntersection(aStart, aEnd, [&](alni start, ualni end, ualni data) {
			out.numFound++;
			out.lastDataFound = data;
		});
		return out;
	};

	intervals.insert({ 2, 5 }, 1);
	intervals.insert({ 12, 15 }, 2);
	intervals.insert({ 22, 25 }, 3);

	TEST(makeQuery(1, 6).isSingleData(1));
	TEST(makeQuery(1, 3).isSingleData(1));
	TEST(makeQuery(3, 6).isSingleData(1));

	TEST(makeQuery(0, 1).notFound());
	TEST(makeQuery(7, 8).notFound());

	TEST(makeQuery(3, 4).isSingleData(1));

	TEST(makeQuery(13, 14).isSingleData(2));

	TEST(makeQuery(1, 35).found(3));
	TEST(makeQuery(11, 35).found(2));

	// check opened
	TEST(makeQuery(5, 12).found(2));
	TEST(makeQuery(15, 22).found(2));

	TEST(makeQuery(1, 2).isSingleData(1));
	TEST(makeQuery(25, 35).isSingleData(3));

	intervals.removeAll();

	intervals.insert({ 0, 3 }, 1);
	intervals.insert({ 0, 13 }, 2);

	TEST(makeQuery(0, 1).found(2));
}

TEST_DEF(IntervalTree) {
	testFunctionalitySimple();
	testFunctionalityScale();
	testEfficency();
	testFunctionalityComplex();
}
