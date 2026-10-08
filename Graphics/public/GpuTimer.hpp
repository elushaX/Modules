#pragma once

#include "Profiler.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace tp {

	class GpuTimer {
	public:
		static constexpr uint32_t kSlots = 8;

		GpuTimer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamily, const char* name);
		~GpuTimer();

		GpuTimer(const GpuTimer&) = delete;
		GpuTimer& operator=(const GpuTimer&) = delete;

		[[nodiscard]] bool isSupported() const { return mPool != VK_NULL_HANDLE; }

		void begin(VkCommandBuffer commandBuffer);
		void end(VkCommandBuffer commandBuffer);
		void collect();

		[[nodiscard]] double lastMs() const { return mLastMs; }

	private:
		VkDevice mDevice;
		VkQueryPool mPool = VK_NULL_HANDLE;
		double mTickNs = 1;
		uint64_t mValidMask = ~0ull;
		ProfileId mSection;

		std::vector<bool> mPending;
		uint32_t mNextSlot = 0;
		uint32_t mOpenSlot = UINT32_MAX;
		double mLastMs = 0;
	};
}
