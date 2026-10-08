#pragma once

#include "VulkanContext.hpp"
#include "Mat.hpp"

#include <vulkan/vulkan.h>

namespace tp {

	struct ObjectPushConstants {
		Mat4F mvp;
		Vec4F offset;
	};

	class VulkanObjectPipeline {
	public:
		VulkanObjectPipeline(VulkanContext* context, VkRenderPass renderPass,
							 VkSampleCountFlagBits samples, bool solidColor);
		~VulkanObjectPipeline();

		VulkanObjectPipeline(const VulkanObjectPipeline&) = delete;
		VulkanObjectPipeline& operator=(const VulkanObjectPipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void pushUniforms(VkCommandBuffer commandBuffer, const ObjectPushConstants& uniforms);

	private:
		VkShaderModule createShaderModule(const uint32_t* code, size_t sizeBytes);

		VulkanContext* mContext = nullptr;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkPipeline mPipeline = VK_NULL_HANDLE;
	};
}
