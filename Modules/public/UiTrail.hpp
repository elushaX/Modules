#pragma once

#include <string>
#include <vector>

namespace tp {

	class UiTrail {
	public:
		static constexpr int kDepth = 32;

		static void push(const char* name);
		static void pop();
		[[nodiscard]] static bool isTracked();
		static void trackCallingThread();
		static void stopTracking();
		[[nodiscard]] static std::vector<std::string> snapshot();
	};
}
