#include "VulkanRenderTarget.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace tp {

	namespace {

		VkSampleCountFlagBits toSampleCount(uint32_t samples) {
			if (samples >= 8) return VK_SAMPLE_COUNT_8_BIT;
			if (samples >= 4) return VK_SAMPLE_COUNT_4_BIT;
			if (samples >= 2) return VK_SAMPLE_COUNT_2_BIT;
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	VulkanRenderTarget::VulkanRenderTarget(VulkanContext* context, int width, int height, uint32_t samples)
		: mContext(context) {
		mExtent = {static_cast<uint32_t>(std::max(width, 1)), static_cast<uint32_t>(std::max(height, 1))};
		mSampleCount = toSampleCount(samples);

		createRenderPass();
		createSampler();
		createResources();
	}

	VulkanRenderTarget::~VulkanRenderTarget() {
		mContext->waitIdle();
		destroyResources();
		if (mSampler != VK_NULL_HANDLE) vkDestroySampler(mContext->device(), mSampler, nullptr);
		if (mRenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(mContext->device(), mRenderPass, nullptr);
	}

	void VulkanRenderTarget::createRenderPass() {
		const bool multisampled = mSampleCount != VK_SAMPLE_COUNT_1_BIT;

		std::vector<VkAttachmentDescription> attachments;

		VkAttachmentDescription colorAttachment{
			.format = mColorFormat,
			.samples = mSampleCount,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = multisampled ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
										: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		attachments.push_back(colorAttachment);

		VkAttachmentDescription depthAttachment{
			.format = mDepthFormat,
			.samples = mSampleCount,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};
		attachments.push_back(depthAttachment);

		if (multisampled) {
			VkAttachmentDescription resolveAttachment{
				.format = mColorFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};
			attachments.push_back(resolveAttachment);
		}

		VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		VkAttachmentReference resolveRef{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorRef,
			.pResolveAttachments = multisampled ? &resolveRef : nullptr,
			.pDepthStencilAttachment = &depthRef,
		};

		const std::array<VkSubpassDependency, 2> dependencies = {
			VkSubpassDependency{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			},
			VkSubpassDependency{
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			},
		};

		VkRenderPassCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = static_cast<uint32_t>(dependencies.size()),
			.pDependencies = dependencies.data(),
		};
		if (vkCreateRenderPass(mContext->device(), &createInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
			throw std::runtime_error("render target: failed to create render pass");
		}
	}

	void VulkanRenderTarget::createSampler() {
		VkSamplerCreateInfo samplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.maxLod = 1.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		};
		if (vkCreateSampler(mContext->device(), &samplerInfo, nullptr, &mSampler) != VK_SUCCESS) {
			throw std::runtime_error("render target: failed to create sampler");
		}
	}

	void VulkanRenderTarget::createResources() {
		const VkDevice device = mContext->device();
		const bool multisampled = mSampleCount != VK_SAMPLE_COUNT_1_BIT;

		auto createImage = [&](VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples,
							   VkImageAspectFlags aspect, VkImage& image, VkDeviceMemory& memory,
							   VkImageView& view) {
			VkImageCreateInfo imageInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = {mExtent.width, mExtent.height, 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = samples,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = usage,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};
			if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
				throw std::runtime_error("render target: failed to create image");
			}

			VkMemoryRequirements requirements{};
			vkGetImageMemoryRequirements(device, image, &requirements);
			VkMemoryAllocateInfo allocateInfo{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = requirements.size,
				.memoryTypeIndex = mContext->findMemoryType(requirements.memoryTypeBits,
															VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
			};
			if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
				throw std::runtime_error("render target: failed to allocate image memory");
			}
			vkBindImageMemory(device, image, memory, 0);

			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = format,
				.subresourceRange = {aspect, 0, 1, 0, 1},
			};
			if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
				throw std::runtime_error("render target: failed to create image view");
			}
		};

		VkImageUsageFlags colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (!multisampled) colorUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		createImage(mColorFormat, colorUsage, mSampleCount, VK_IMAGE_ASPECT_COLOR_BIT,
					mColorImage, mColorMemory, mColorView);

		createImage(mDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, mSampleCount,
					VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					mDepthImage, mDepthMemory, mDepthView);

		std::vector<VkImageView> framebufferViews = {mColorView, mDepthView};

		if (multisampled) {
			createImage(mColorFormat,
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
							VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
						VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
						mResolveImage, mResolveMemory, mResolveView);
			framebufferViews.push_back(mResolveView);
		}

		VkFramebufferCreateInfo framebufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = mRenderPass,
			.attachmentCount = static_cast<uint32_t>(framebufferViews.size()),
			.pAttachments = framebufferViews.data(),
			.width = mExtent.width,
			.height = mExtent.height,
			.layers = 1,
		};
		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFramebuffer) != VK_SUCCESS) {
			throw std::runtime_error("render target: failed to create framebuffer");
		}
	}

	void VulkanRenderTarget::destroyResources() {
		const VkDevice device = mContext->device();

		if (mFramebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, mFramebuffer, nullptr);

		auto destroyImage = [&](VkImage& image, VkDeviceMemory& memory, VkImageView& view) {
			if (view != VK_NULL_HANDLE) vkDestroyImageView(device, view, nullptr);
			if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
			if (memory != VK_NULL_HANDLE) vkFreeMemory(device, memory, nullptr);
			view = VK_NULL_HANDLE;
			image = VK_NULL_HANDLE;
			memory = VK_NULL_HANDLE;
		};

		destroyImage(mColorImage, mColorMemory, mColorView);
		destroyImage(mDepthImage, mDepthMemory, mDepthView);
		destroyImage(mResolveImage, mResolveMemory, mResolveView);
		mFramebuffer = VK_NULL_HANDLE;
	}

	void VulkanRenderTarget::resize(int width, int height) {
		const VkExtent2D next{static_cast<uint32_t>(std::max(width, 1)), static_cast<uint32_t>(std::max(height, 1))};
		if (next.width == mExtent.width && next.height == mExtent.height) return;

		mContext->waitIdle();
		destroyResources();
		mExtent = next;
		createResources();
	}

	void VulkanRenderTarget::beginRenderPass(VkCommandBuffer commandBuffer, const VkClearColorValue& clearColor) {
		std::array<VkClearValue, 2> clears{};
		clears[0].color = clearColor;
		clears[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = mRenderPass,
			.framebuffer = mFramebuffer,
			.renderArea = {{0, 0}, mExtent},
			.clearValueCount = static_cast<uint32_t>(clears.size()),
			.pClearValues = clears.data(),
		};
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(mExtent.width),
			.height = static_cast<float>(mExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{{0, 0}, mExtent};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VulkanRenderTarget::endRenderPass(VkCommandBuffer commandBuffer) {
		vkCmdEndRenderPass(commandBuffer);
	}
}
