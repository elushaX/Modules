#pragma once

#include "Environment.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tp {

	using ProfileId = uint32_t;

	struct FrameProfile {
		uint64_t index = 0;
		double wallMs = 0;
		double cpuMs = 0;
		std::vector<double> sectionMs;
		std::vector<uint32_t> sectionCalls;
		std::vector<double> counters;
		std::vector<double> gpuMs;
	};

	struct ProfileStatistics {
		size_t samples = 0;
		double total = 0;
		double mean = 0;
		double minimum = 0;
		double median = 0;
		double p95 = 0;
		double p99 = 0;
		double maximum = 0;
	};

	[[nodiscard]] ProfileStatistics summarize(std::vector<double> values);

	class Profiler {
	public:
		static constexpr size_t kMaxNames = 128;

		[[nodiscard]] static Profiler& global();

		void setEnabled(bool enabled);
		[[nodiscard]] bool isEnabled() const { return mEnabled; }

		[[nodiscard]] ProfileId section(const char* name);
		[[nodiscard]] ProfileId counter(const char* name);
		[[nodiscard]] ProfileId gpuSection(const char* name);

		void beginFrame();
		void endFrame();
		[[nodiscard]] bool isInFrame() const { return mInFrame; }

		void addSectionTime(ProfileId section, double milliseconds);
		void addCounter(ProfileId counter, double amount);
		void addGpuTime(ProfileId section, double milliseconds);

		void clear();

		[[nodiscard]] const std::vector<FrameProfile>& frames() const { return mFrames; }
		[[nodiscard]] const std::vector<std::string>& sectionNames() const { return mSectionNames; }
		[[nodiscard]] const std::vector<std::string>& counterNames() const { return mCounterNames; }
		[[nodiscard]] const std::vector<std::string>& gpuSectionNames() const { return mGpuNames; }

		[[nodiscard]] std::vector<double> sectionSeries(const std::string& name) const;
		[[nodiscard]] std::vector<double> counterSeries(const std::string& name) const;
		[[nodiscard]] std::vector<double> gpuSeries(const std::string& name) const;
		[[nodiscard]] double counterTotal(const std::string& name) const;
		[[nodiscard]] double sectionTotal(const std::string& name) const;
		[[nodiscard]] uint64_t sectionCallTotal(const std::string& name) const;

		[[nodiscard]] static double threadCpuMs();
		[[nodiscard]] static double processCpuMs();
		[[nodiscard]] static double peakResidentMegabytes();

	private:
		static ProfileId intern(std::vector<std::string>& names, const char* name);
		[[nodiscard]] static ptrdiff_t indexOf(const std::vector<std::string>& names, const std::string& name);

		bool mEnabled = false;
		bool mInFrame = false;
		uint64_t mNextFrame = 0;
		double mFrameWallStart = 0;
		double mFrameCpuStart = 0;

		std::vector<std::string> mSectionNames;
		std::vector<std::string> mCounterNames;
		std::vector<std::string> mGpuNames;

		std::array<double, kMaxNames> mSectionMs{};
		std::array<uint32_t, kMaxNames> mSectionCalls{};
		std::array<double, kMaxNames> mCounters{};
		std::array<double, kMaxNames> mGpuMs{};

		std::vector<FrameProfile> mFrames;
	};

	class ProfileScope {
	public:
		ProfileScope(ProfileId section, const char* name);
		~ProfileScope();

		ProfileScope(const ProfileScope&) = delete;
		ProfileScope& operator=(const ProfileScope&) = delete;

	private:
		ProfileId mSection;
		int64_t mStartNs = -1;
	};

	inline void profileCount(ProfileId counter, double amount = 1) {
		Profiler& profiler = Profiler::global();
		if (profiler.isEnabled()) profiler.addCounter(counter, amount);
	}
}

#define TP_PROFILE_CONCAT_INNER(a, b) a##b
#define TP_PROFILE_CONCAT(a, b) TP_PROFILE_CONCAT_INNER(a, b)

#define TP_PROFILE_SCOPE(name)                                                                                    \
	static const ::tp::ProfileId TP_PROFILE_CONCAT(tpProfileId, __LINE__) = ::tp::Profiler::global().section(name); \
	const ::tp::ProfileScope TP_PROFILE_CONCAT(tpProfileScope, __LINE__)(TP_PROFILE_CONCAT(tpProfileId, __LINE__), name)

#define TP_PROFILE_COUNT(name, amount)                                                        \
	do {                                                                                        \
		static const ::tp::ProfileId tpProfileCounterId = ::tp::Profiler::global().counter(name); \
		::tp::profileCount(tpProfileCounterId, (double) (amount));                                \
	} while (false)
