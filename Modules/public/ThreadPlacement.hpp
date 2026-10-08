#pragma once

#include <cstddef>
#include <vector>

namespace tp {

	class ThreadPlacement {
	public:
		struct Layout {
			std::vector<int> interactive;
			std::vector<int> workers;
		};

		[[nodiscard]] static Layout layoutFor(int interactiveCpu, const std::vector<std::vector<int>>& siblingGroups);

		static void reserveCallingThreadForInteraction();
		static void placeCallingThreadAsWorker();

		[[nodiscard]] static Layout current();
		[[nodiscard]] static std::vector<int> callingThreadCpus();
		[[nodiscard]] static size_t workerCount();
	};
}
