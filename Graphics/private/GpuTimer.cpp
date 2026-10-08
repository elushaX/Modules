#include "GpuTimer.hpp"

#include <vector>

namespace tp {

	GpuTimer::GpuTimer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamily, const char* name) :
		mDevice(device),
		mSection(Profiler::global().gpuSection(name)),
		mPending(kSlots, false) {
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		uint32_t familyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
		std::vector<VkQueueFamilyProperties> families(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

		const uint32_t validBits = queueFamily < familyCount ? families[queueFamily].timestampValidBits : 0;
		if (validBits == 0 || properties.limits.timestampPeriod <= 0.0f) return;

		mTickNs = properties.limits.timestampPeriod;
		mValidMask = validBits >= 64 ? ~0ull : ((1ull << validBits) - 1ull);

		VkQueryPoolCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			.queryType = VK_QUERY_TYPE_TIMESTAMP,
			.queryCount = kSlots * 2,
		};
		if (vkCreateQueryPool(mDevice, &info, nullptr, &mPool) != VK_SUCCESS) mPool = VK_NULL_HANDLE;
	}

	GpuTimer::~GpuTimer() {
		if (mPool != VK_NULL_HANDLE) vkDestroyQueryPool(mDevice, mPool, nullptr);
	}

	void GpuTimer::begin(VkCommandBuffer commandBuffer) {
		mOpenSlot = UINT32_MAX;
		if (!isSupported() || !Profiler::global().isEnabled()) return;

		collect();
		const uint32_t slot = mNextSlot;
		if (mPending[slot]) return;

		vkCmdResetQueryPool(commandBuffer, mPool, slot * 2, 2);
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mPool, slot * 2);
		mOpenSlot = slot;
	}

	void GpuTimer::end(VkCommandBuffer commandBuffer) {
		if (mOpenSlot == UINT32_MAX) return;
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mPool, mOpenSlot * 2 + 1);
		mPending[mOpenSlot] = true;
		mNextSlot = (mOpenSlot + 1) % kSlots;
		mOpenSlot = UINT32_MAX;
	}

	void GpuTimer::collect() {
		if (!isSupported()) return;
		for (uint32_t offset = 0; offset < kSlots; offset++) {
			const uint32_t slot = (mNextSlot + offset) % kSlots;
			if (!mPending[slot]) continue;

			uint64_t stamps[2] = {};
			const VkResult result = vkGetQueryPoolResults(
				mDevice, mPool, slot * 2, 2, sizeof(stamps), stamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT
			);
			if (result != VK_SUCCESS) continue;

			mPending[slot] = false;
			const uint64_t ticks = (stamps[1] - stamps[0]) & mValidMask;
			mLastMs = (double) ticks * mTickNs / 1.0e6;
			Profiler::global().addGpuTime(mSection, mLastMs);
		}
	}
}
