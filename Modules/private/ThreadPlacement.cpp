#include "ThreadPlacement.hpp"

#include "Environment.hpp"

#include <algorithm>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>

#if defined(ENV_OS_LINUX) || defined(__ANDROID__)
#include <sched.h>
#define TP_THREAD_PLACEMENT_SUPPORTED
#endif

namespace tp {

	namespace {
		std::vector<int> parseCpuList(const std::string& text) {
			std::vector<int> cpus;
			std::stringstream stream(text);
			std::string range;
			while (std::getline(stream, range, ',')) {
				if (range.empty()) continue;
				const size_t dash = range.find('-');
				const int first = std::stoi(range.substr(0, dash));
				const int last = dash == std::string::npos ? first : std::stoi(range.substr(dash + 1));
				for (int cpu = first; cpu <= last; cpu++)
					cpus.push_back(cpu);
			}
			return cpus;
		}

#ifdef TP_THREAD_PLACEMENT_SUPPORTED
		std::vector<int> allowedCpus() {
			cpu_set_t set;
			CPU_ZERO(&set);
			if (sched_getaffinity(0, sizeof(set), &set) != 0) return {};
			std::vector<int> cpus;
			for (int cpu = 0; cpu < CPU_SETSIZE; cpu++)
				if (CPU_ISSET(cpu, &set)) cpus.push_back(cpu);
			return cpus;
		}

		std::vector<int> siblingsOf(int cpu) {
			std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/thread_siblings_list");
			std::string text;
			if (!file || !std::getline(file, text)) return { cpu };
			std::vector<int> siblings = parseCpuList(text);
			return siblings.empty() ? std::vector<int>{ cpu } : siblings;
		}

		std::vector<std::vector<int>> siblingGroups(const std::vector<int>& allowed) {
			std::vector<std::vector<int>> groups;
			std::set<int> seen;
			for (int cpu : allowed) {
				if (seen.count(cpu)) continue;
				std::vector<int> group;
				for (int sibling : siblingsOf(cpu)) {
					if (!std::count(allowed.begin(), allowed.end(), sibling)) continue;
					group.push_back(sibling);
					seen.insert(sibling);
				}
				if (group.empty()) group.push_back(cpu);
				groups.push_back(group);
			}
			return groups;
		}

		void pinCallingThread(const std::vector<int>& cpus) {
			if (cpus.empty()) return;
			cpu_set_t set;
			CPU_ZERO(&set);
			for (int cpu : cpus)
				CPU_SET(cpu, &set);
			sched_setaffinity(0, sizeof(set), &set);
		}
#endif

		struct Reservation {
			std::mutex mutex;
			bool made = false;
			ThreadPlacement::Layout layout;
		};

		Reservation& reservation() {
			static Reservation shared;
			return shared;
		}
	}

	ThreadPlacement::Layout
	ThreadPlacement::layoutFor(int interactiveCpu, const std::vector<std::vector<int>>& siblingGroups) {
		Layout layout;
		for (const std::vector<int>& group : siblingGroups) {
			const bool owned = std::count(group.begin(), group.end(), interactiveCpu) > 0;
			std::vector<int>& target = owned ? layout.interactive : layout.workers;
			target.insert(target.end(), group.begin(), group.end());
		}
		if (layout.workers.empty()) layout.workers = layout.interactive;
		return layout;
	}

	void ThreadPlacement::reserveCallingThreadForInteraction() {
#ifdef TP_THREAD_PLACEMENT_SUPPORTED
		const std::vector<int> allowed = allowedCpus();
		if (allowed.size() < 2) return;
		const int running = sched_getcpu();
		const int interactive = std::count(allowed.begin(), allowed.end(), running) ? running : allowed.front();
		Layout layout = layoutFor(interactive, siblingGroups(allowed));
		Reservation& shared = reservation();
		{
			std::lock_guard lock(shared.mutex);
			shared.layout = layout;
			shared.made = true;
		}
		pinCallingThread(layout.interactive);
#endif
	}

	void ThreadPlacement::placeCallingThreadAsWorker() {
#ifdef TP_THREAD_PLACEMENT_SUPPORTED
		std::vector<int> workers;
		{
			Reservation& shared = reservation();
			std::lock_guard lock(shared.mutex);
			if (!shared.made) return;
			workers = shared.layout.workers;
		}
		pinCallingThread(workers);
#endif
	}

	ThreadPlacement::Layout ThreadPlacement::current() {
		Reservation& shared = reservation();
		std::lock_guard lock(shared.mutex);
		return shared.made ? shared.layout : Layout{};
	}

	std::vector<int> ThreadPlacement::callingThreadCpus() {
#ifdef TP_THREAD_PLACEMENT_SUPPORTED
		return allowedCpus();
#else
		return {};
#endif
	}

	size_t ThreadPlacement::workerCount() {
		const Layout layout = current();
		if (!layout.workers.empty() && layout.workers != layout.interactive)
			return std::max<size_t>(2, layout.workers.size());
		const unsigned hardware = std::thread::hardware_concurrency();
		return hardware > 2 ? (size_t) hardware - 1 : 2;
	}
}
