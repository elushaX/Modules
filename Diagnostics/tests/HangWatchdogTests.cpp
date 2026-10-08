#include "HangWatchdog.hpp"
#include "Profiler.hpp"

#include "UnitTest++/UnitTest++.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

using namespace tp;

namespace {
	struct TempDirectory {
		std::filesystem::path path;

		explicit TempDirectory(const std::string& name) :
			path(std::filesystem::temp_directory_path() / ("hang-watchdog-test-" + name)) {
			std::filesystem::remove_all(path);
		}
		~TempDirectory() {
			if (!std::getenv("KEEP_HANG_REPORTS")) std::filesystem::remove_all(path);
		}
	};

	HangSettings fastSettings(const std::filesystem::path& directory) {
		HangSettings settings;
		settings.thresholdMs = 150;
		settings.followUpMs = {};
		settings.stackSamples = 3;
		settings.stackSampleIntervalMs = 30;
		settings.application = "test";
		settings.build = "0";
		settings.directory = directory;
		return settings;
	}

	std::string readFile(const std::filesystem::path& path) {
		std::ifstream file(path);
		std::stringstream text;
		text << file.rdbuf();
		return text.str();
	}

	bool hasVerdict(const std::filesystem::path& report) {
		return readFile(report / "summary.txt").find("verdict:") != std::string::npos;
	}

	template <typename Done>
	bool waitUntil(Done&& done, double limitMs = 5000) {
		const auto start = std::chrono::steady_clock::now();
		while (!done()) {
			if (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() > limitMs)
				return false;
			std::this_thread::yield();
		}
		return true;
	}

	class Gate {
	public:
		void waitOpen() {
			std::unique_lock lock(mMutex);
			mOpened.wait(lock, [this]() { return mOpen; });
		}
		void open() {
			{
				std::lock_guard lock(mMutex);
				mOpen = true;
			}
			mOpened.notify_all();
		}

	private:
		std::mutex mMutex;
		std::condition_variable mOpened;
		bool mOpen = false;
	};
}

SUITE(HangWatchdog) {
	TEST(StuckDrawWritesAReportNamingTheOpenScopes) {
		TempDirectory directory("stuck");
		HangWatchdog watchdog(fastSettings(directory.path));
		Gate gate;
		watchdog.enterPhase(UiPhase::Draw);
		{
			TP_PROFILE_SCOPE("test.outer");
			TP_PROFILE_SCOPE("test.waiting_on_gate");
			std::thread release([&]() {
				waitUntil([&]() { return watchdog.reportsWritten() > 0 && hasVerdict(watchdog.reports().front()); });
				gate.open();
			});
			gate.waitOpen();
			release.join();
		}
		watchdog.enterPhase(UiPhase::Idle);
		REQUIRE CHECK_EQUAL(1u, watchdog.reports().size());
		const std::string summary = readFile(watchdog.reports().front() / "summary.txt");
		CHECK(summary.find("drawing") != std::string::npos);
		CHECK(summary.find("test.outer") != std::string::npos);
		CHECK(summary.find("test.waiting_on_gate") != std::string::npos);
		CHECK(summary.find("verdict: blocked") != std::string::npos);
		const std::string threads = readFile(watchdog.reports().front() / "threads.txt");
		CHECK(threads.find("UI THREAD") != std::string::npos);
		CHECK(
			threads.find("waitOpen") != std::string::npos || threads.find("pthread_cond") != std::string::npos ||
			threads.find("libc") != std::string::npos
		);
		CHECK(waitUntil([&]() {
			return readFile(watchdog.reports().front() / "summary.txt").find("recovered") != std::string::npos;
		}));
	}

	TEST(SpinningUiThreadIsReportedAsBusy) {
		TempDirectory directory("busy");
		HangWatchdog watchdog(fastSettings(directory.path));
		watchdog.enterPhase(UiPhase::Input);
		{
			TP_PROFILE_SCOPE("test.spinning");
			volatile uint64_t work = 0;
			while (watchdog.reportsWritten() == 0 || !hasVerdict(watchdog.reports().front()))
				work = work + 1;
		}
		watchdog.enterPhase(UiPhase::Idle);
		const std::string summary = readFile(watchdog.reports().front() / "summary.txt");
		CHECK(summary.find("test.spinning") != std::string::npos);
		CHECK(summary.find("verdict: busy") != std::string::npos);
	}

	TEST(AProcessPauseIsNotCountedAsAHang) {
		TempDirectory directory("paused");
		std::atomic<int64_t> clock{ 0 };
		HangSettings settings = fastSettings(directory.path);
		settings.thresholdMs = 2000;
		settings.processPauseMs = 1000;
		settings.clockNs = [&clock]() { return clock.load(); };
		HangWatchdog watchdog(settings);
		watchdog.enterPhase(UiPhase::Draw);

		const auto step = [&](int64_t milliseconds) {
			const uint64_t seen = watchdog.inspections();
			clock.fetch_add(milliseconds * 1000000);
			REQUIRE CHECK(waitUntil([&]() { return watchdog.inspections() > seen; }));
		};
		step(100);
		step(30000);
		step(100);
		CHECK_EQUAL(0u, watchdog.reportsWritten());

		for (int tick = 0; tick < 25; tick++)
			step(100);
		CHECK_EQUAL(1u, watchdog.reportsWritten());
		watchdog.enterPhase(UiPhase::Idle);
	}

	TEST(SummaryIsWrittenBeforeThreadsAreSampled) {
		TempDirectory directory("summary-first");
		HangSettings settings = fastSettings(directory.path);
		std::filesystem::path seenReport;
		bool summaryExisted = false;
		HangWatchdog watchdog(settings);
		watchdog.addContributor([&]() {
			seenReport = watchdog.reports().empty() ? std::filesystem::path() : watchdog.reports().front();
			summaryExisted = !seenReport.empty() && std::filesystem::exists(seenReport / "summary.txt");
			return std::vector<HangReportSection>{};
		});
		watchdog.enterPhase(UiPhase::Present);
		REQUIRE CHECK(waitUntil([&]() { return watchdog.reportsWritten() > 0; }));
		watchdog.enterPhase(UiPhase::Idle);
		CHECK(summaryExisted);
		const std::string summary = readFile(seenReport / "summary.txt");
		CHECK(summary.find("debugger attached no") != std::string::npos);
	}

	TEST(InputEventsHandledInOnePhaseAreCounted) {
		TempDirectory directory("input-loop");
		HangWatchdog watchdog(fastSettings(directory.path));
		watchdog.enterPhase(UiPhase::Input);
		{
			TP_PROFILE_SCOPE("test.input_loop");
			while (watchdog.reportsWritten() == 0 || !hasVerdict(watchdog.reports().front()))
				watchdog.noteInputEvent();
		}
		watchdog.enterPhase(UiPhase::Idle);
		const std::string summary = readFile(watchdog.reports().front() / "summary.txt");
		CHECK(summary.find("input events handled during this phase 0") == std::string::npos);
		CHECK(summary.find("input events were handled without leaving the input phase") != std::string::npos);
	}

	TEST(IdleNeverProducesAReport) {
		TempDirectory directory("idle");
		HangWatchdog watchdog(fastSettings(directory.path));
		watchdog.enterPhase(UiPhase::Idle);
		Gate gate;
		std::thread release([&]() {
			const auto start = std::chrono::steady_clock::now();
			waitUntil([&]() { return std::chrono::steady_clock::now() - start > std::chrono::milliseconds(600); });
			gate.open();
		});
		gate.waitOpen();
		release.join();
		CHECK_EQUAL(0u, watchdog.reportsWritten());
	}

	TEST(ContributorsAddTheirOwnFiles) {
		TempDirectory directory("contributors");
		HangWatchdog watchdog(fastSettings(directory.path));
		watchdog.addContributor([]() { return std::vector<HangReportSection>{ { "engine.txt", "queue 3\n" } }; });
		watchdog.enterPhase(UiPhase::Input);
		REQUIRE CHECK(waitUntil([&]() { return watchdog.reportsWritten() > 0; }));
		watchdog.enterPhase(UiPhase::Idle);
		CHECK_EQUAL(std::string("queue 3\n"), readFile(watchdog.reports().front() / "engine.txt"));
	}

	TEST(OnlyTheNewestReportsAreKept) {
		TempDirectory directory("retention");
		std::filesystem::create_directories(directory.path);
		for (int index = 0; index < 5; index++)
			std::filesystem::create_directories(directory.path / ("old-" + std::to_string(index)));
		HangSettings settings = fastSettings(directory.path);
		settings.keptReports = 2;
		HangWatchdog watchdog(settings);
		CHECK_EQUAL(2u, watchdog.reports().size());
	}

	TEST(DirectoryIsTheApplicationsHangFolderUnlessOverridden) {
		const std::filesystem::path application = "/home/someone/sketch2d/hangs";
		CHECK(HangWatchdog::directoryFor(application) == application);
		setenv("MODULES_HANG_DIR", "/tmp/custom-hangs", 1);
		CHECK(HangWatchdog::directoryFor(application) == std::filesystem::path("/tmp/custom-hangs"));
		unsetenv("MODULES_HANG_DIR");
	}
}
