#pragma once

#include "StackTrace.hpp"
#include "UiTrail.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tp {

	enum class UiPhase : uint8_t { Idle, Input, Draw, Present };

	struct HangSettings {
		double thresholdMs = 2000;
		std::vector<double> followUpMs = { 10000, 30000 };
		int stackSamples = 5;
		double stackSampleIntervalMs = 200;
		double processPauseMs = 1000;
		size_t keptReports = 20;
		std::string application = "app";
		std::string build = "dev";
		std::filesystem::path directory;
		std::function<int64_t()> clockNs;
	};

	struct HangReportSection {
		std::string fileName;
		std::string text;
	};

	class HangWatchdog {
	public:
		using Contributor = std::function<std::vector<HangReportSection>()>;

		explicit HangWatchdog(HangSettings settings);
		~HangWatchdog();

		HangWatchdog(const HangWatchdog&) = delete;
		HangWatchdog& operator=(const HangWatchdog&) = delete;

		void enterPhase(UiPhase phase);
		void noteInputEvent();
		void addContributor(Contributor contributor);

		[[nodiscard]] const std::filesystem::path& directory() const { return mSettings.directory; }
		[[nodiscard]] std::vector<std::filesystem::path> reports() const;
		[[nodiscard]] size_t reportsWritten() const { return mReportsWritten.load(); }
		[[nodiscard]] uint64_t inspections() const { return mInspections.load(); }

		[[nodiscard]] static std::filesystem::path defaultHangsDirectory();
		[[nodiscard]] static std::filesystem::path directoryFor(const std::filesystem::path& applicationHangsDirectory);
		[[nodiscard]] static std::string processName();
		static void allowDebuggerAttach();
		[[nodiscard]] static const char* phaseName(UiPhase phase);

	private:
		struct Stall {
			uint64_t beat = 0;
			UiPhase phase = UiPhase::Idle;
			std::filesystem::path report;
			size_t followUps = 0;
		};

		struct Investigation {
			std::string verdict;
			std::string threads;
		};

		void watch();
		void inspect(Stall& stall, int64_t now);
		void discountProcessPause(int64_t now, int64_t previous);
		[[nodiscard]] double stalledMs(int64_t now) const;
		[[nodiscard]] uint64_t inputEventsInPhase() const;

		void beginReport(Stall& stall, double stalled);
		void appendFollowUp(Stall& stall, double stalled);
		void finishRecovered(Stall& stall, double stalled);
		[[nodiscard]] std::string reportHeader() const;
		[[nodiscard]] std::string uiSnapshot(double stalled, UiPhase phase) const;
		void writeContributions(const std::filesystem::path& report);
		[[nodiscard]] Investigation investigate() const;
		[[nodiscard]] std::string
		sampleStacks(std::vector<std::vector<uintptr_t>>& uiSamples, std::string& uiState, std::string& uiWait) const;
		[[nodiscard]] static std::string formatThreads(const std::vector<ThreadSample>& threads, int uiTid);
		[[nodiscard]] static std::string stackVerdict(
			const std::vector<std::vector<uintptr_t>>& uiSamples, const std::string& state, const std::string& waitChannel
		);

		void pruneOldReports() const;
		[[nodiscard]] std::filesystem::path newReportPath() const;
		[[nodiscard]] int64_t now() const;

		HangSettings mSettings;
		std::atomic<uint64_t> mBeat{ 0 };
		std::atomic<int64_t> mPhaseStartNs{ 0 };
		std::atomic<UiPhase> mPhase{ UiPhase::Idle };
		std::atomic<uint64_t> mInputEvents{ 0 };
		std::atomic<uint64_t> mPhaseInputEvents{ 0 };
		std::atomic<size_t> mReportsWritten{ 0 };
		std::atomic<uint64_t> mInspections{ 0 };
		int mUiTid = 0;

		uint64_t mPausedBeat = 0;
		int64_t mPausedNs = 0;

		std::mutex mContributorsMutex;
		std::vector<Contributor> mContributors;

		std::mutex mStopMutex;
		std::condition_variable mStopped;
		bool mStopping = false;
		std::thread mThread;
	};
}
