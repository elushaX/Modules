#pragma once

#include "VulkanContext.hpp"

#include <vulkan/vulkan.h>

namespace tp {

	class VulkanRenderTarget {
	public:
		VulkanRenderTarget(VulkanContext* context, int width, int height, uint32_t samples);
		~VulkanRenderTarget();

		VulkanRenderTarget(const VulkanRenderTarget&) = delete;
		VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

		void resize(int width, int height);

		void beginRenderPass(VkCommandBuffer commandBuffer, const VkClearColorValue& clearColor);
		void endRenderPass(VkCommandBuffer commandBuffer);

		VkRenderPass renderPass() const { return mRenderPass; }
		VkExtent2D extent() const { return mExtent; }
		VkSampleCountFlagBits sampleCount() const { return mSampleCount; }

		VkImageView resolvedColorView() const { return mResolveView != VK_NULL_HANDLE ? mResolveView : mColorView; }
		VkSampler sampler() const { return mSampler; }
		VkImage resolvedColorImage() const { return mResolveImage != VK_NULL_HANDLE ? mResolveImage : mColorImage; }

	private:
		void createResources();
		void destroyResources();
		void createRenderPass();
		void createSampler();

		VulkanContext* mContext = nullptr;
		VkExtent2D mExtent{};
		VkSampleCountFlagBits mSampleCount = VK_SAMPLE_COUNT_1_BIT;

		VkFormat mColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
		VkFormat mDepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

		VkImage mColorImage = VK_NULL_HANDLE;
		VkDeviceMemory mColorMemory = VK_NULL_HANDLE;
		VkImageView mColorView = VK_NULL_HANDLE;

		VkImage mResolveImage = VK_NULL_HANDLE;
		VkDeviceMemory mResolveMemory = VK_NULL_HANDLE;
		VkImageView mResolveView = VK_NULL_HANDLE;

		VkImage mDepthImage = VK_NULL_HANDLE;
		VkDeviceMemory mDepthMemory = VK_NULL_HANDLE;
		VkImageView mDepthView = VK_NULL_HANDLE;

		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE;
	};
}
