#include "Profiler.hpp"
#include "UiTrail.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <sys/resource.h>

namespace tp {

	namespace {
		double clockMs(clockid_t clock) {
			timespec now{};
			clock_gettime(clock, &now);
			return (double) now.tv_sec * 1000.0 + (double) now.tv_nsec / 1.0e6;
		}

		int64_t monotonicNs() {
			timespec now{};
			clock_gettime(CLOCK_MONOTONIC, &now);
			return (int64_t) now.tv_sec * 1000000000ll + now.tv_nsec;
		}

		double percentile(const std::vector<double>& sorted, double fraction) {
			if (sorted.empty()) return 0;
			const double position = fraction * (double) (sorted.size() - 1);
			const size_t lower = (size_t) std::floor(position);
			const size_t upper = std::min(lower + 1, sorted.size() - 1);
			const double blend = position - (double) lower;
			return sorted[lower] + (sorted[upper] - sorted[lower]) * blend;
		}
	}

	ProfileStatistics summarize(std::vector<double> values) {
		ProfileStatistics statistics;
		statistics.samples = values.size();
		if (values.empty()) return statistics;

		std::sort(values.begin(), values.end());
		for (double value : values)
			statistics.total += value;
		statistics.mean = statistics.total / (double) values.size();
		statistics.minimum = values.front();
		statistics.maximum = values.back();
		statistics.median = percentile(values, 0.5);
		statistics.p95 = percentile(values, 0.95);
		statistics.p99 = percentile(values, 0.99);
		return statistics;
	}

	Profiler& Profiler::global() {
		static Profiler profiler;
		return profiler;
	}

	void Profiler::setEnabled(bool enabled) {
		mEnabled = enabled;
		if (!enabled) mInFrame = false;
	}

	ProfileId Profiler::intern(std::vector<std::string>& names, const char* name) {
		const ptrdiff_t existing = indexOf(names, name);
		if (existing >= 0) return (ProfileId) existing;
		if (names.size() >= kMaxNames) return (ProfileId) (kMaxNames - 1);
		names.emplace_back(name);
		return (ProfileId) (names.size() - 1);
	}

	ptrdiff_t Profiler::indexOf(const std::vector<std::string>& names, const std::string& name) {
		const auto found = std::find(names.begin(), names.end(), name);
		return found == names.end() ? -1 : found - names.begin();
	}

	ProfileId Profiler::section(const char* name) { return intern(mSectionNames, name); }

	ProfileId Profiler::counter(const char* name) { return intern(mCounterNames, name); }

	ProfileId Profiler::gpuSection(const char* name) { return intern(mGpuNames, name); }

	void Profiler::beginFrame() {
		if (!mEnabled) return;
		mSectionMs.fill(0);
		mSectionCalls.fill(0);
		mCounters.fill(0);
		mGpuMs.fill(0);
		mFrameWallStart = clockMs(CLOCK_MONOTONIC);
		mFrameCpuStart = threadCpuMs();
		mInFrame = true;
	}

	void Profiler::endFrame() {
		if (!mEnabled || !mInFrame) return;
		mInFrame = false;

		FrameProfile frame;
		frame.index = mNextFrame++;
		frame.wallMs = clockMs(CLOCK_MONOTONIC) - mFrameWallStart;
		frame.cpuMs = threadCpuMs() - mFrameCpuStart;
		frame.sectionMs.assign(mSectionMs.begin(), mSectionMs.begin() + (ptrdiff_t) mSectionNames.size());
		frame.sectionCalls.assign(mSectionCalls.begin(), mSectionCalls.begin() + (ptrdiff_t) mSectionNames.size());
		frame.counters.assign(mCounters.begin(), mCounters.begin() + (ptrdiff_t) mCounterNames.size());
		frame.gpuMs.assign(mGpuMs.begin(), mGpuMs.begin() + (ptrdiff_t) mGpuNames.size());
		mFrames.push_back(std::move(frame));
	}

	void Profiler::addSectionTime(ProfileId section, double milliseconds) {
		if (!mInFrame || section >= kMaxNames) return;
		mSectionMs[section] += milliseconds;
		mSectionCalls[section]++;
	}

	void Profiler::addCounter(ProfileId counter, double amount) {
		if (!mInFrame || counter >= kMaxNames) return;
		mCounters[counter] += amount;
	}

	void Profiler::addGpuTime(ProfileId section, double milliseconds) {
		if (!mInFrame || section >= kMaxNames) return;
		mGpuMs[section] += milliseconds;
	}

	void Profiler::clear() {
		mFrames.clear();
		mNextFrame = 0;
		mInFrame = false;
	}

	std::vector<double> Profiler::sectionSeries(const std::string& name) const {
		std::vector<double> series;
		const ptrdiff_t index = indexOf(mSectionNames, name);
		if (index < 0) return series;
		for (const FrameProfile& frame : mFrames)
			series.push_back((size_t) index < frame.sectionMs.size() ? frame.sectionMs[(size_t) index] : 0.0);
		return series;
	}

	std::vector<double> Profiler::counterSeries(const std::string& name) const {
		std::vector<double> series;
		const ptrdiff_t index = indexOf(mCounterNames, name);
		if (index < 0) return series;
		for (const FrameProfile& frame : mFrames)
			series.push_back((size_t) index < frame.counters.size() ? frame.counters[(size_t) index] : 0.0);
		return series;
	}

	std::vector<double> Profiler::gpuSeries(const std::string& name) const {
		std::vector<double> series;
		const ptrdiff_t index = indexOf(mGpuNames, name);
		if (index < 0) return series;
		for (const FrameProfile& frame : mFrames)
			series.push_back((size_t) index < frame.gpuMs.size() ? frame.gpuMs[(size_t) index] : 0.0);
		return series;
	}

	double Profiler::counterTotal(const std::string& name) const {
		double total = 0;
		for (double value : counterSeries(name))
			total += value;
		return total;
	}

	double Profiler::sectionTotal(const std::string& name) const {
		double total = 0;
		for (double value : sectionSeries(name))
			total += value;
		return total;
	}

	uint64_t Profiler::sectionCallTotal(const std::string& name) const {
		uint64_t total = 0;
		const ptrdiff_t index = indexOf(mSectionNames, name);
		if (index < 0) return 0;
		for (const FrameProfile& frame : mFrames) {
			if ((size_t) index < frame.sectionCalls.size()) total += frame.sectionCalls[(size_t) index];
		}
		return total;
	}

	double Profiler::threadCpuMs() { return clockMs(CLOCK_THREAD_CPUTIME_ID); }

	double Profiler::processCpuMs() { return clockMs(CLOCK_PROCESS_CPUTIME_ID); }

	double Profiler::peakResidentMegabytes() {
		rusage usage{};
		getrusage(RUSAGE_SELF, &usage);
		return (double) usage.ru_maxrss / 1024.0;
	}

	ProfileScope::ProfileScope(ProfileId section, const char* name) :
		mSection(section) {
		UiTrail::push(name);
		if (Profiler::global().isInFrame()) mStartNs = monotonicNs();
	}

	ProfileScope::~ProfileScope() {
		UiTrail::pop();
		if (mStartNs < 0) return;
		Profiler::global().addSectionTime(mSection, (double) (monotonicNs() - mStartNs) / 1.0e6);
	}
}
