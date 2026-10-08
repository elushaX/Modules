#include "Profiler.hpp"

#include "UnitTest++/UnitTest++.h"

#include <cmath>

using namespace tp;

namespace {
	void burnCpu(double milliseconds) {
		const double end = Profiler::threadCpuMs() + milliseconds;
		volatile double sink = 0;
		while (Profiler::threadCpuMs() < end)
			sink = sink + std::sqrt(sink + 1.0);
	}

	struct ProfilerScope {
		Profiler profiler;
		ProfilerScope() { profiler.setEnabled(true); }
	};
}

SUITE(ProfileStatistics) {
	TEST(SummaryReportsPercentilesOfUnsortedValues) {
		std::vector<double> values;
		for (int value = 100; value >= 1; value--)
			values.push_back(value);

		const ProfileStatistics statistics = summarize(values);
		CHECK_EQUAL(100u, statistics.samples);
		CHECK_CLOSE(50.5, statistics.mean, 1e-9);
		CHECK_CLOSE(50.5, statistics.median, 1e-9);
		CHECK_CLOSE(95.05, statistics.p95, 1e-9);
		CHECK_CLOSE(99.01, statistics.p99, 1e-9);
		CHECK_EQUAL(1.0, statistics.minimum);
		CHECK_EQUAL(100.0, statistics.maximum);
		CHECK_EQUAL(5050.0, statistics.total);
	}

	TEST(EmptySummaryIsZero) {
		const ProfileStatistics statistics = summarize({});
		CHECK_EQUAL(0u, statistics.samples);
		CHECK_EQUAL(0.0, statistics.p99);
	}
}

SUITE(Profiler) {
	TEST(DisabledProfilerRecordsNothing) {
		Profiler profiler;
		const ProfileId section = profiler.section("work");
		profiler.beginFrame();
		profiler.addSectionTime(section, 5);
		profiler.endFrame();
		CHECK(profiler.frames().empty());
	}

	TEST(FramesAccumulateSectionsCallsAndCounters) {
		ProfilerScope scope;
		Profiler& profiler = scope.profiler;
		const ProfileId dabs = profiler.counter("dabs");
		const ProfileId stamp = profiler.section("stamp");

		profiler.beginFrame();
		profiler.addSectionTime(stamp, 1.5);
		profiler.addSectionTime(stamp, 0.5);
		profiler.addCounter(dabs, 3);
		profiler.endFrame();
		profiler.beginFrame();
		profiler.addCounter(dabs, 4);
		profiler.endFrame();

		REQUIRE CHECK_EQUAL(2u, profiler.frames().size());
		CHECK_EQUAL(2.0, profiler.frames()[0].sectionMs[stamp]);
		CHECK_EQUAL(2u, profiler.frames()[0].sectionCalls[stamp]);
		CHECK_EQUAL(0.0, profiler.frames()[1].sectionMs[stamp]);
		CHECK_EQUAL(7.0, profiler.counterTotal("dabs"));
		CHECK_EQUAL(2u, profiler.sectionCallTotal("stamp"));
		CHECK_EQUAL(1u, profiler.frames()[1].index);
	}

	TEST(WorkOutsideAFrameIsIgnored) {
		ProfilerScope scope;
		const ProfileId dabs = scope.profiler.counter("dabs");
		scope.profiler.addCounter(dabs, 9);
		scope.profiler.beginFrame();
		scope.profiler.endFrame();
		CHECK_EQUAL(0.0, scope.profiler.counterTotal("dabs"));
	}

	TEST(SectionsInternByName) {
		Profiler profiler;
		const ProfileId first = profiler.section("draw");
		CHECK_EQUAL(first, profiler.section("draw"));
		CHECK(first != profiler.section("process"));
	}

	TEST(FrameMeasuresWallAndThreadCpuTime) {
		ProfilerScope scope;
		scope.profiler.beginFrame();
		burnCpu(4);
		scope.profiler.endFrame();
		const FrameProfile& frame = scope.profiler.frames().front();
		CHECK(frame.cpuMs >= 4.0);
		CHECK(frame.wallMs >= frame.cpuMs * 0.9);
	}

	TEST(ScopeMacroTimesTheGlobalProfiler) {
		Profiler& profiler = Profiler::global();
		profiler.clear();
		profiler.setEnabled(true);
		profiler.beginFrame();
		{
			TP_PROFILE_SCOPE("macro scope");
			burnCpu(2);
		}
		TP_PROFILE_COUNT("macro counter", 2);
		profiler.endFrame();
		profiler.setEnabled(false);

		CHECK(profiler.sectionTotal("macro scope") >= 2.0);
		CHECK_EQUAL(1u, profiler.sectionCallTotal("macro scope"));
		CHECK_EQUAL(2.0, profiler.counterTotal("macro counter"));
		profiler.clear();
	}

	TEST(GpuTimesArePerFrame) {
		ProfilerScope scope;
		const ProfileId canvas = scope.profiler.gpuSection("canvas");
		scope.profiler.beginFrame();
		scope.profiler.addGpuTime(canvas, 0.75);
		scope.profiler.endFrame();
		REQUIRE CHECK_EQUAL(1u, scope.profiler.gpuSeries("canvas").size());
		CHECK_EQUAL(0.75, scope.profiler.gpuSeries("canvas")[0]);
	}
}
