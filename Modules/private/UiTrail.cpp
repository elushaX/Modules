#include "UiTrail.hpp"

#include <algorithm>
#include <atomic>
#include <thread>

namespace tp {

	namespace {
		struct Trail {
			std::atomic<const char*> names[UiTrail::kDepth] = {};
			std::atomic<int> depth{ 0 };
			std::atomic<int> overflow{ 0 };
		};

		Trail& trail() {
			static Trail shared;
			return shared;
		}

		std::atomic<std::thread::id>& trackedThread() {
			static std::atomic<std::thread::id> tracked{};
			return tracked;
		}
	}

	void UiTrail::push(const char* name) {
		if (!isTracked()) return;
		Trail& shared = trail();
		const int depth = shared.depth.load(std::memory_order_relaxed);
		if (depth < kDepth) shared.names[depth].store(name, std::memory_order_relaxed);
		else shared.overflow.fetch_add(1, std::memory_order_relaxed);
		shared.depth.store(depth + 1, std::memory_order_release);
	}

	void UiTrail::pop() {
		if (!isTracked()) return;
		Trail& shared = trail();
		const int depth = shared.depth.load(std::memory_order_relaxed);
		if (depth <= 0) return;
		if (depth > kDepth) shared.overflow.fetch_sub(1, std::memory_order_relaxed);
		shared.depth.store(depth - 1, std::memory_order_release);
	}

	bool UiTrail::isTracked() { return trackedThread().load(std::memory_order_relaxed) == std::this_thread::get_id(); }

	void UiTrail::trackCallingThread() {
		trail().depth.store(0);
		trail().overflow.store(0);
		trackedThread().store(std::this_thread::get_id());
	}

	void UiTrail::stopTracking() { trackedThread().store(std::thread::id{}); }

	std::vector<std::string> UiTrail::snapshot() {
		Trail& shared = trail();
		const int depth = std::min(shared.depth.load(std::memory_order_acquire), kDepth);
		std::vector<std::string> names;
		for (int index = 0; index < depth; index++) {
			const char* name = shared.names[index].load(std::memory_order_relaxed);
			names.emplace_back(name ? name : "?");
		}
		const int overflow = shared.overflow.load(std::memory_order_relaxed);
		if (overflow > 0) names.push_back("... " + std::to_string(overflow) + " deeper");
		return names;
	}
}
