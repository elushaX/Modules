#include "HangWatchdog.hpp"

#include "AppStorage.hpp"
#include "StackTrace.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

#if defined(__linux__)
#include <sys/prctl.h>
#endif

namespace tp {

	namespace {
		int64_t nowNs() {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
				.count();
		}

		std::string timestamp(const char* pattern) {
			const std::time_t now = std::time(nullptr);
			char text[64];
			std::strftime(text, sizeof(text), pattern, std::localtime(&now));
			return text;
		}

		void writeFile(const std::filesystem::path& path, const std::string& text, std::ios::openmode mode) {
			std::ofstream file(path, mode);
			file << text;
		}

		constexpr double kCheckIntervalMs = 100;
	}

	HangWatchdog::HangWatchdog(HangSettings settings) :
		mSettings(std::move(settings)) {
		mSettings.directory = directoryFor(mSettings.directory.empty() ? defaultHangsDirectory() : mSettings.directory);
		if (!mSettings.clockNs) mSettings.clockNs = nowNs;
		mPhaseStartNs.store(now());
		allowDebuggerAttach();
		StackTrace::prepareThreadSampling();
		mUiTid = StackTrace::callingThreadId();
		UiTrail::trackCallingThread();
		pruneOldReports();
		mThread = std::thread([this]() { watch(); });
	}

	HangWatchdog::~HangWatchdog() {
		{
			std::lock_guard lock(mStopMutex);
			mStopping = true;
		}
		mStopped.notify_all();
		mThread.join();
		UiTrail::stopTracking();
	}

	std::string HangWatchdog::formatThreads(const std::vector<ThreadSample>& threads, int uiTid) {
		std::ostringstream text;
		for (const ThreadSample& thread : threads) {
			text << (thread.tid == uiTid ? "UI THREAD " : "thread ") << thread.tid << " \"" << thread.name << "\" state "
					 << thread.state;
			if (!thread.waitChannel.empty() && thread.waitChannel != "0") text << " waiting in " << thread.waitChannel;
			text << "\n";
			if (!thread.answered) text << "  (did not answer the sample signal)\n";
			for (uintptr_t frame : thread.frames)
				text << "  " << StackTrace::describe(frame) << "\n";
			text << "\n";
		}
		return text.str();
	}

	void HangWatchdog::enterPhase(UiPhase phase) {
		mPhaseStartNs.store(now(), std::memory_order_relaxed);
		mPhaseInputEvents.store(mInputEvents.load(std::memory_order_relaxed), std::memory_order_relaxed);
		mPhase.store(phase, std::memory_order_relaxed);
		mBeat.fetch_add(1, std::memory_order_release);
	}

	void HangWatchdog::noteInputEvent() { mInputEvents.fetch_add(1, std::memory_order_relaxed); }

	int64_t HangWatchdog::now() const { return mSettings.clockNs(); }

	void HangWatchdog::addContributor(Contributor contributor) {
		std::lock_guard lock(mContributorsMutex);
		mContributors.push_back(std::move(contributor));
	}

	std::filesystem::path HangWatchdog::defaultHangsDirectory() {
		return AppStorage::inHome(processName()).hangsDirectory();
	}

	std::filesystem::path HangWatchdog::directoryFor(const std::filesystem::path& applicationHangsDirectory) {
		if (const char* configured = std::getenv("MODULES_HANG_DIR"); configured && *configured) return configured;
		return applicationHangsDirectory;
	}

	std::string HangWatchdog::processName() {
#if defined(__linux__)
		std::ifstream comm("/proc/self/comm");
		std::string name;
		if (std::getline(comm, name) && !name.empty()) return name;
#endif
		return "app";
	}

	void HangWatchdog::allowDebuggerAttach() {
#if defined(__linux__) && defined(PR_SET_PTRACER)
		prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
#endif
	}

	const char* HangWatchdog::phaseName(UiPhase phase) {
		switch (phase) {
			case UiPhase::Idle: return "idle";
			case UiPhase::Input: return "processing input";
			case UiPhase::Draw: return "drawing";
			case UiPhase::Present: return "presenting";
		}
		return "unknown";
	}

	void HangWatchdog::watch() {
		Stall stall;
		int64_t previous = now();
		std::unique_lock lock(mStopMutex);
		while (!mStopping) {
			mStopped.wait_for(lock, std::chrono::duration<double, std::milli>(kCheckIntervalMs), [this]() {
				return mStopping;
			});
			if (mStopping) return;
			lock.unlock();
			const int64_t current = now();
			discountProcessPause(current, previous);
			inspect(stall, current);
			previous = current;
			mInspections.fetch_add(1);
			lock.lock();
		}
	}

	void HangWatchdog::discountProcessPause(int64_t now, int64_t previous) {
		const double sinceLastCheckMs = (double) (now - previous) / 1.0e6;
		const double pausedMs = sinceLastCheckMs - kCheckIntervalMs;
		if (pausedMs < mSettings.processPauseMs) return;

		const uint64_t beat = mBeat.load(std::memory_order_acquire);
		if (beat != mPausedBeat) {
			mPausedBeat = beat;
			mPausedNs = 0;
		}
		mPausedNs += (int64_t) (pausedMs * 1.0e6);
	}

	double HangWatchdog::stalledMs(int64_t now) const {
		const bool pausedThisPhase = mPausedBeat == mBeat.load(std::memory_order_acquire);
		const int64_t paused = pausedThisPhase ? mPausedNs : 0;
		return (double) (now - mPhaseStartNs.load(std::memory_order_relaxed) - paused) / 1.0e6;
	}

	uint64_t HangWatchdog::inputEventsInPhase() const {
		return mInputEvents.load(std::memory_order_relaxed) - mPhaseInputEvents.load(std::memory_order_relaxed);
	}

	void HangWatchdog::inspect(Stall& stall, int64_t now) {
		const uint64_t beat = mBeat.load(std::memory_order_acquire);
		const UiPhase phase = mPhase.load(std::memory_order_relaxed);
		const double stalled = stalledMs(now);

		if (!stall.report.empty() && stall.beat != beat) {
			finishRecovered(stall, stalled);
			stall = {};
		}
		if (phase == UiPhase::Idle || stalled < mSettings.thresholdMs) return;

		if (stall.report.empty()) {
			stall.beat = beat;
			stall.phase = phase;
			beginReport(stall, stalled);
			return;
		}
		if (stall.followUps < mSettings.followUpMs.size() && stalled >= mSettings.followUpMs[stall.followUps])
			appendFollowUp(stall, stalled);
	}

	std::string HangWatchdog::uiSnapshot(double stalled, UiPhase phase) const {
		std::ostringstream text;
		text << "ui thread " << phaseName(phase) << " for " << (int64_t) stalled << " ms\n";
		text << "input events handled during this phase " << inputEventsInPhase() << "\n";
		text << "scope trail (outermost first):\n";
		const std::vector<std::string> names = UiTrail::snapshot();
		if (names.empty()) text << "  (no profiled scope open)\n";
		for (size_t index = 0; index < names.size(); index++)
			text << "  " << std::string(index * 2, ' ') << names[index] << "\n";
		return text.str();
	}

	std::filesystem::path HangWatchdog::newReportPath() const {
		const std::string base = mSettings.application + "-" + timestamp("%Y-%m-%d_%H-%M-%S") + "-build" + mSettings.build;
		std::filesystem::path path = mSettings.directory / base;
		for (int suffix = 2; std::filesystem::exists(path); suffix++)
			path = mSettings.directory / (base + "-" + std::to_string(suffix));
		return path;
	}

	std::string HangWatchdog::reportHeader() const {
		std::ostringstream text;
		text << "application " << mSettings.application << "\n";
		text << "build " << mSettings.build << "\n";
		text << "detected " << timestamp("%Y-%m-%d %H:%M:%S") << "\n";
		text << "threshold " << (int64_t) mSettings.thresholdMs << " ms\n";
		text << "debugger attached " << (StackTrace::isDebuggerAttached() ? "yes" : "no") << "\n\n";
		return text.str();
	}

	void HangWatchdog::beginReport(Stall& stall, double stalled) {
		std::error_code error;
		std::filesystem::create_directories(mSettings.directory, error);
		stall.report = newReportPath();
		std::filesystem::create_directories(stall.report, error);
		if (error) return;

		writeFile(
			stall.report / "summary.txt", reportHeader() + uiSnapshot(stalled, stall.phase), std::ios::out | std::ios::trunc
		);
		writeContributions(stall.report);
		mReportsWritten.fetch_add(1);

		const Investigation investigation = investigate();
		writeFile(
			stall.report / "summary.txt", "\nverdict: " + investigation.verdict + "\n", std::ios::out | std::ios::app
		);
		if (!investigation.threads.empty())
			writeFile(stall.report / "threads.txt", investigation.threads, std::ios::out | std::ios::trunc);
		pruneOldReports();
	}

	void HangWatchdog::writeContributions(const std::filesystem::path& report) {
		std::vector<Contributor> contributors;
		{
			std::lock_guard lock(mContributorsMutex);
			contributors = mContributors;
		}
		for (const Contributor& contributor : contributors)
			for (const HangReportSection& section : contributor())
				writeFile(report / section.fileName, section.text, std::ios::out | std::ios::trunc);
	}

	void HangWatchdog::appendFollowUp(Stall& stall, double stalled) {
		stall.followUps++;
		const std::string header = "\nstill stuck at " + timestamp("%H:%M:%S") + "\n";
		writeFile(stall.report / "summary.txt", header + uiSnapshot(stalled, stall.phase), std::ios::out | std::ios::app);

		const Investigation investigation = investigate();
		writeFile(stall.report / "summary.txt", "verdict: " + investigation.verdict + "\n", std::ios::out | std::ios::app);
		if (!investigation.threads.empty())
			writeFile(
				stall.report / "threads.txt",
				"\n===== follow-up at " + timestamp("%H:%M:%S") + " =====\n" + investigation.threads,
				std::ios::out | std::ios::app
			);
	}

	void HangWatchdog::finishRecovered(Stall& stall, double stalled) {
		(void) stalled;
		writeFile(
			stall.report / "summary.txt", "\nrecovered at " + timestamp("%H:%M:%S") + "\n", std::ios::out | std::ios::app
		);
	}

	HangWatchdog::Investigation HangWatchdog::investigate() const {
		if (StackTrace::isDebuggerAttached())
			return { "not sampled: a debugger is attached, its thread view shows where the ui thread is", "" };
		if (!StackTrace::canSampleThreads()) return { "not sampled: thread stacks are not available on this platform", "" };

		std::vector<std::vector<uintptr_t>> uiSamples;
		std::string uiState;
		std::string uiWait;
		std::string threads = sampleStacks(uiSamples, uiState, uiWait);
		std::string verdict = stackVerdict(uiSamples, uiState, uiWait);
		if (inputEventsInPhase() > 0)
			verdict +=
				"; " + std::to_string(inputEventsInPhase()) + " input events were handled without leaving the input phase";
		return { verdict, threads };
	}

	std::string HangWatchdog::sampleStacks(
		std::vector<std::vector<uintptr_t>>& uiSamples, std::string& uiState, std::string& uiWait
	) const {
		std::ostringstream text;
		for (int sample = 0; sample < std::max(1, mSettings.stackSamples); sample++) {
			if (sample > 0)
				std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(mSettings.stackSampleIntervalMs));
			const std::vector<ThreadSample> threads = StackTrace::sampleAllThreads();
			for (const ThreadSample& thread : threads)
				if (thread.tid == mUiTid) {
					uiSamples.push_back(thread.frames);
					uiState = thread.state;
					uiWait = thread.waitChannel;
				}
			text << "===== sample " << sample + 1 << " =====\n" << formatThreads(threads, mUiTid);
		}
		return text.str();
	}

	std::string HangWatchdog::stackVerdict(
		const std::vector<std::vector<uintptr_t>>& uiSamples, const std::string& state, const std::string& waitChannel
	) {
		if (uiSamples.empty()) return "ui thread not found";
		const bool sameStack =
			std::all_of(uiSamples.begin(), uiSamples.end(), [&](const auto& frames) { return frames == uiSamples.front(); });
		const bool sleeping = !state.empty() && (state[0] == 'S' || state[0] == 'D');
		if (sameStack && sleeping) {
			std::string text = "blocked: the ui thread sleeps at the same place in every sample";
			if (!waitChannel.empty() && waitChannel != "0") text += " (kernel wait " + waitChannel + ")";
			return text;
		}
		if (!sleeping) return "busy: the ui thread is running, check the changing frames in threads.txt";
		return "waiting repeatedly: the ui thread sleeps but at different places, likely polling or lock contention";
	}

	std::vector<std::filesystem::path> HangWatchdog::reports() const {
		std::vector<std::filesystem::path> found;
		std::error_code error;
		for (const auto& entry : std::filesystem::directory_iterator(mSettings.directory, error))
			if (entry.is_directory()) found.push_back(entry.path());
		std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
			return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
		});
		return found;
	}

	void HangWatchdog::pruneOldReports() const {
		const std::vector<std::filesystem::path> found = reports();
		if (found.size() <= mSettings.keptReports) return;
		std::error_code error;
		for (size_t index = 0; index + mSettings.keptReports < found.size(); index++)
			std::filesystem::remove_all(found[index], error);
	}
}
