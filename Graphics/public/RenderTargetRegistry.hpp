#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace tp {

	struct RenderTargetEntry {
		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		int width = 0;
		int height = 0;
	};

	class RenderTargetRegistry {
	public:
		static uint32_t registerTarget(const RenderTargetEntry& entry);
		static void updateTarget(uint32_t id, const RenderTargetEntry& entry);
		static void unregisterTarget(uint32_t id);
		static bool resolve(uint32_t id, RenderTargetEntry& out);
	};
}
