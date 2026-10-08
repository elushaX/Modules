#include <cstdio>
#include "VulkanContext.hpp"
#include "GpuTimer.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace tp {

	namespace {

		constexpr std::array<const char*, 1> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		bool validationRequested() {
			const char* value = std::getenv("MODULES_VULKAN_VALIDATION");
			return value && std::strcmp(value, "1") == 0;
		}

		bool hasValidationLayer() {
			uint32_t count = 0;
			vkEnumerateInstanceLayerProperties(&count, nullptr);
			std::vector<VkLayerProperties> layers(count);
			vkEnumerateInstanceLayerProperties(&count, layers.data());
			for (const auto& layer : layers) {
				if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) return true;
			}
			return false;
		}

		bool deviceSupportsExtensions(VkPhysicalDevice device) {
			uint32_t count = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
			std::vector<VkExtensionProperties> available(count);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());
			for (const char* required : kDeviceExtensions) {
				bool found = false;
				for (const auto& extension : available) {
					if (std::strcmp(required, extension.extensionName) == 0) {
						found = true;
						break;
					}
				}
				if (!found) return false;
			}
			return true;
		}
	}

	namespace {
		VulkanContext* g_currentContext = nullptr;
	}

	VulkanContext* VulkanContext::current() { return g_currentContext; }

	VulkanContext::VulkanContext(VulkanSurfaceSource& surfaceSource) :
		mSurfaceSource(surfaceSource) {
		createInstance();
		createSurface();
		selectPhysicalDevice();
		selectCompositeAlpha();
		createLogicalDevice();
		createRenderPass();
		createCommandResources();
		createSyncObjects();
		mFrameGpuTimer = std::make_unique<GpuTimer>(mDevice, mPhysicalDevice, mGraphicsQueueFamily, "gpu.window");

		g_currentContext = this;

		const VkExtent2D extent = mSurfaceSource.framebufferExtent();
		createSwapchain(std::max<int>((int) extent.width, 1), std::max<int>((int) extent.height, 1));
		mRequestedExtent = extent;
	}

	VulkanContext::~VulkanContext() {
		if (g_currentContext == this) g_currentContext = nullptr;

		if (mDevice != VK_NULL_HANDLE) vkDeviceWaitIdle(mDevice);
		mFrameGpuTimer.reset();

		destroySwapchain();

		for (auto semaphore : mImageAvailableSemaphores)
			vkDestroySemaphore(mDevice, semaphore, nullptr);
		for (auto semaphore : mRenderFinishedSemaphores)
			vkDestroySemaphore(mDevice, semaphore, nullptr);
		for (auto fence : mInFlightFences)
			vkDestroyFence(mDevice, fence, nullptr);

		if (mCommandPool != VK_NULL_HANDLE) vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
		if (mRenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
		if (mDevice != VK_NULL_HANDLE) vkDestroyDevice(mDevice, nullptr);
		if (mSurface != VK_NULL_HANDLE) vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		if (mInstance != VK_NULL_HANDLE) vkDestroyInstance(mInstance, nullptr);
	}

	void VulkanContext::createInstance() {
		std::vector<const char*> extensions = mSurfaceSource.requiredInstanceExtensions();

		std::vector<const char*> layers;
		if (validationRequested() && hasValidationLayer()) layers.push_back("VK_LAYER_KHRONOS_validation");

		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Modules",
			.apiVersion = VK_API_VERSION_1_2,
		};

		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};

		if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vulkan instance");
		}
	}

	void VulkanContext::createSurface() {
		mSurface = mSurfaceSource.createSurface(mInstance);
		if (mSurface == VK_NULL_HANDLE) throw std::runtime_error("failed to create window surface");
	}

	void VulkanContext::selectCompositeAlpha() {
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &capabilities);

		mCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		if (!(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
			for (VkCompositeAlphaFlagBitsKHR candidate : { VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
																										 VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
																										 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR }) {
				if (capabilities.supportedCompositeAlpha & candidate) {
					mCompositeAlpha = candidate;
					break;
				}
			}
		}
		if (
			mSurfaceSource.wantsTranslucency() &&
			(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		) {
			mCompositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
			mTranslucent = true;
		}

		mSurfaceSource.onTranslucencyResolved(isTranslucent());
	}

	void VulkanContext::selectPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
		if (deviceCount == 0) throw std::runtime_error("no vulkan capable gpu found");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

		for (auto device : devices) {
			if (!deviceSupportsExtensions(device)) continue;

			uint32_t familyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
			std::vector<VkQueueFamilyProperties> families(familyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

			uint32_t graphics = UINT32_MAX;
			uint32_t present = UINT32_MAX;
			for (uint32_t i = 0; i < familyCount; i++) {
				if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphics = i;
				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
				if (presentSupport) present = i;
			}

			if (graphics == UINT32_MAX || present == UINT32_MAX) continue;

			mPhysicalDevice = device;
			mGraphicsQueueFamily = graphics;
			mPresentQueueFamily = present;
			return;
		}

		throw std::runtime_error("no suitable vulkan device");
	}

	void VulkanContext::createLogicalDevice() {
		const float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueInfos;

		VkDeviceQueueCreateInfo graphicsInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = mGraphicsQueueFamily,
			.queueCount = 1,
			.pQueuePriorities = &priority,
		};
		queueInfos.push_back(graphicsInfo);

		if (mPresentQueueFamily != mGraphicsQueueFamily) {
			VkDeviceQueueCreateInfo presentInfo = graphicsInfo;
			presentInfo.queueFamilyIndex = mPresentQueueFamily;
			queueInfos.push_back(presentInfo);
		}

		VkPhysicalDeviceFeatures supported{};
		vkGetPhysicalDeviceFeatures(mPhysicalDevice, &supported);
		VkPhysicalDeviceFeatures features{};
		features.geometryShader = supported.geometryShader;

		VkDeviceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
			.pQueueCreateInfos = queueInfos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size()),
			.ppEnabledExtensionNames = kDeviceExtensions.data(),
			.pEnabledFeatures = &features,
		};

		if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device");
		}

		vkGetDeviceQueue(mDevice, mGraphicsQueueFamily, 0, &mGraphicsQueue);
		vkGetDeviceQueue(mDevice, mPresentQueueFamily, 0, &mPresentQueue);
	}

	void VulkanContext::createRenderPass() {
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, formats.data());

		mSwapchainFormat = formats.front().format;
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				mSwapchainFormat = format.format;
				break;
			}
		}

		mDepthStencilFormat = selectDepthStencilFormat();

		const std::array<VkAttachmentDescription, 2> attachments = {
			VkAttachmentDescription{
				.format = mSwapchainFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			},
			VkAttachmentDescription{
				.format = mDepthStencilFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			},
		};

		VkAttachmentReference colorRef{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depthRef{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorRef,
			.pDepthStencilAttachment = &depthRef,
		};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		VkRenderPassCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		if (vkCreateRenderPass(mDevice, &createInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass");
		}
	}

	VkFormat VulkanContext::selectDepthStencilFormat() const {
		const std::array<VkFormat, 3> candidates = {
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
		};
		for (VkFormat format : candidates) {
			VkFormatProperties properties{};
			vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &properties);
			if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				return format;
			}
		}
		throw std::runtime_error("no supported depth-stencil format");
	}

	uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("no suitable memory type for depth-stencil");
	}

	void VulkanContext::createDepthStencil() {
		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = mDepthStencilFormat,
			.extent = { mSwapchainExtent.width, mSwapchainExtent.height, 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		if (vkCreateImage(mDevice, &imageInfo, nullptr, &mDepthStencilImage) != VK_SUCCESS) {
			throw std::runtime_error("failed to create depth-stencil image");
		}

		VkMemoryRequirements requirements{};
		vkGetImageMemoryRequirements(mDevice, mDepthStencilImage, &requirements);

		VkMemoryAllocateInfo allocateInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = requirements.size,
			.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		if (vkAllocateMemory(mDevice, &allocateInfo, nullptr, &mDepthStencilMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate depth-stencil memory");
		}
		vkBindImageMemory(mDevice, mDepthStencilImage, mDepthStencilMemory, 0);

		VkImageViewCreateInfo viewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = mDepthStencilImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = mDepthStencilFormat,
			.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 },
		};
		if (vkCreateImageView(mDevice, &viewInfo, nullptr, &mDepthStencilView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create depth-stencil view");
		}
	}

	void VulkanContext::destroyDepthStencil() {
		if (mDepthStencilView != VK_NULL_HANDLE) vkDestroyImageView(mDevice, mDepthStencilView, nullptr);
		if (mDepthStencilImage != VK_NULL_HANDLE) vkDestroyImage(mDevice, mDepthStencilImage, nullptr);
		if (mDepthStencilMemory != VK_NULL_HANDLE) vkFreeMemory(mDevice, mDepthStencilMemory, nullptr);
		mDepthStencilView = VK_NULL_HANDLE;
		mDepthStencilImage = VK_NULL_HANDLE;
		mDepthStencilMemory = VK_NULL_HANDLE;
	}

	void VulkanContext::createCommandResources() {
		VkCommandPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = mGraphicsQueueFamily,
		};
		if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool");
		}

		mCommandBuffers.resize(kFramesInFlight);
		VkCommandBufferAllocateInfo allocateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = mCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = kFramesInFlight,
		};
		if (vkAllocateCommandBuffers(mDevice, &allocateInfo, mCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void VulkanContext::createSyncObjects() {
		mImageAvailableSemaphores.resize(kFramesInFlight);
		mRenderFinishedSemaphores.resize(kFramesInFlight);
		mInFlightFences.resize(kFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		for (uint32_t i = 0; i < kFramesInFlight; i++) {
			vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]);
			vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]);
			vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]);
		}
	}

	VkPresentModeKHR VulkanContext::selectPresentMode() const {
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &count, nullptr);
		std::vector<VkPresentModeKHR> modes(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &count, modes.data());

		// Mailbox replaces the queued frame instead of waiting behind it, so the
		// compositor always shows the newest frame. FIFO is the guaranteed fallback.
		for (VkPresentModeKHR mode : modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void VulkanContext::createSwapchain(int width, int height, VkSwapchainKHR previous) {
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &capabilities);

		mSwapchainExtent = capabilities.currentExtent;
		mSurfaceTransform = capabilities.currentTransform;
		mSurfaceExtent = capabilities.currentExtent;
		VkSurfaceTransformFlagBitsKHR transform = capabilities.currentTransform;
		if (
			transform != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR &&
			(capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		) {
			transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			const bool quarterTurn = capabilities.currentTransform &
															 (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR);
			if (quarterTurn && mSwapchainExtent.width != std::numeric_limits<uint32_t>::max())
				std::swap(mSwapchainExtent.width, mSwapchainExtent.height);
		}
		if (mSwapchainExtent.width == std::numeric_limits<uint32_t>::max()) {
			mSwapchainExtent.width =
				std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			mSwapchainExtent.height = std::clamp(
				static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height
			);
		}

		mPresentMode = selectPresentMode();
		uint32_t imageCount = capabilities.minImageCount;
		if (mPresentMode == VK_PRESENT_MODE_MAILBOX_KHR) imageCount += 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = mSurface,
			.minImageCount = imageCount,
			.imageFormat = mSwapchainFormat,
			.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			.imageExtent = mSwapchainExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.preTransform = transform,
			.compositeAlpha = mCompositeAlpha,
			.presentMode = mPresentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = previous,
		};

		const uint32_t families[] = { mGraphicsQueueFamily, mPresentQueueFamily };
		if (mGraphicsQueueFamily != mPresentQueueFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = families;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swapchain");
		}
		if (previous != VK_NULL_HANDLE) vkDestroySwapchainKHR(mDevice, previous, nullptr);

		uint32_t actualImageCount = 0;
		vkGetSwapchainImagesKHR(mDevice, mSwapchain, &actualImageCount, nullptr);
		mSwapchainImages.resize(actualImageCount);
		vkGetSwapchainImagesKHR(mDevice, mSwapchain, &actualImageCount, mSwapchainImages.data());

		createDepthStencil();

		mSwapchainImageViews.resize(actualImageCount);
		mSwapchainFramebuffers.resize(actualImageCount);
		for (uint32_t i = 0; i < actualImageCount; i++) {
			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = mSwapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = mSwapchainFormat,
				.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			};
			if (vkCreateImageView(mDevice, &viewInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create swapchain image view");
			}

			const std::array<VkImageView, 2> framebufferAttachments = {
				mSwapchainImageViews[i],
				mDepthStencilView,
			};

			VkFramebufferCreateInfo framebufferInfo{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = mRenderPass,
				.attachmentCount = static_cast<uint32_t>(framebufferAttachments.size()),
				.pAttachments = framebufferAttachments.data(),
				.width = mSwapchainExtent.width,
				.height = mSwapchainExtent.height,
				.layers = 1,
			};
			if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer");
			}
		}
	}

	void VulkanContext::destroySwapchainResources() {
		destroyDepthStencil();
		for (auto framebuffer : mSwapchainFramebuffers)
			vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
		for (auto view : mSwapchainImageViews)
			vkDestroyImageView(mDevice, view, nullptr);

		mSwapchainFramebuffers.clear();
		mSwapchainImageViews.clear();
		mSwapchainImages.clear();
	}

	void VulkanContext::destroySwapchain() {
		destroySwapchainResources();
		if (mSwapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
		mSwapchain = VK_NULL_HANDLE;
	}

	void VulkanContext::recreateSwapchain(int width, int height) {
		// Only the frames using the old swapchain's views must finish; the old
		// swapchain is handed to the new one so presentation never stalls on a gap.
		vkWaitForFences(
			mDevice, static_cast<uint32_t>(mInFlightFences.size()), mInFlightFences.data(), VK_TRUE, UINT64_MAX
		);
		destroySwapchainResources();

		VkSwapchainKHR previous = mSwapchain;
		mSwapchain = VK_NULL_HANDLE;
		createSwapchain(std::max(width, 1), std::max(height, 1), previous);
		mRequestedExtent = { (uint32_t) width, (uint32_t) height };
		mSwapchainRecreations++;
	}

	bool VulkanContext::swapchainMatches(int framebufferWidth, int framebufferHeight) const {
		return mRequestedExtent.width == static_cast<uint32_t>(framebufferWidth) &&
					 mRequestedExtent.height == static_cast<uint32_t>(framebufferHeight);
	}

	bool VulkanContext::beginFrame(int framebufferWidth, int framebufferHeight) {
		if (framebufferWidth <= 0 || framebufferHeight <= 0) return false;
		if (mSurface == VK_NULL_HANDLE) return false;

		if (!swapchainMatches(framebufferWidth, framebufferHeight)) {
			recreateSwapchain(framebufferWidth, framebufferHeight);
		}

		uint32_t imageIndex = 0;
		VkResult acquire = VK_SUCCESS;
		{
			TP_PROFILE_SCOPE("frame.acquire");
			vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
			acquire = vkAcquireNextImageKHR(
				mDevice, mSwapchain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex
			);
		}

		if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain(framebufferWidth, framebufferHeight);
			return false;
		}
		if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swapchain image");
		}

		vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);
		mAcquiredImageIndex = imageIndex;

		VkCommandBuffer commandBuffer = mCommandBuffers[mCurrentFrame];
		vkResetCommandBuffer(commandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		mFrameGpuTimer->begin(commandBuffer);

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, isTranslucent() ? 0.0f : 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = mRenderPass,
			.framebuffer = mSwapchainFramebuffers[imageIndex],
			.renderArea = { .offset = { 0, 0 }, .extent = mSwapchainExtent },
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data(),
		};
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(mSwapchainExtent.width),
			.height = static_cast<float>(mSwapchainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{ { 0, 0 }, mSwapchainExtent };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		mFrameActive = true;
		return true;
	}

	void VulkanContext::endFrame() {
		if (!mFrameActive) return;

		TP_PROFILE_SCOPE("frame.submit");
		VkCommandBuffer commandBuffer = mCommandBuffers[mCurrentFrame];
		vkCmdEndRenderPass(commandBuffer);
		mFrameGpuTimer->end(commandBuffer);
		vkEndCommandBuffer(commandBuffer);

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &mImageAvailableSemaphores[mCurrentFrame],
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &mRenderFinishedSemaphores[mCurrentFrame],
		};
		vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]);

		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrame],
			.swapchainCount = 1,
			.pSwapchains = &mSwapchain,
			.pImageIndices = &mAcquiredImageIndex,
		};
		VkResult present = vkQueuePresentKHR(mPresentQueue, &presentInfo);

		if (present == VK_ERROR_OUT_OF_DATE_KHR || (present == VK_SUBOPTIMAL_KHR && surfaceChanged())) {
			const VkExtent2D extent = mSurfaceSource.framebufferExtent();
			recreateSwapchain((int) extent.width, (int) extent.height);
		}

		mCurrentFrame = (mCurrentFrame + 1) % static_cast<uint32_t>(mCommandBuffers.size());
		mFrameActive = false;
	}

	bool VulkanContext::surfaceChanged() const {
		VkSurfaceCapabilitiesKHR capabilities{};
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &capabilities) != VK_SUCCESS) return true;
		return capabilities.currentTransform != mSurfaceTransform ||
					 capabilities.currentExtent.width != mSurfaceExtent.width ||
					 capabilities.currentExtent.height != mSurfaceExtent.height;
	}

	void VulkanContext::waitIdle() {
		if (mDevice != VK_NULL_HANDLE) vkDeviceWaitIdle(mDevice);
	}

	void VulkanContext::releaseSurface() {
		if (mSurface == VK_NULL_HANDLE) return;
		vkDeviceWaitIdle(mDevice);
		destroySwapchain();
		mSwapchainExtent = { 0, 0 };
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		mSurface = VK_NULL_HANDLE;
	}

	void VulkanContext::recreateSurface() {
		if (mSurface != VK_NULL_HANDLE) return;
		createSurface();
		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, mPresentQueueFamily, mSurface, &presentSupport);
		if (!presentSupport) throw std::runtime_error("recreated surface cannot be presented");
		const VkExtent2D extent = mSurfaceSource.framebufferExtent();
		createSwapchain(std::max<int>((int) extent.width, 1), std::max<int>((int) extent.height, 1));
	}

	void VulkanContext::syncSwapchainToWindow() {
		const VkExtent2D extent = mSurfaceSource.framebufferExtent();
		if (extent.width == 0 || extent.height == 0) return;
		recreateSwapchain((int) extent.width, (int) extent.height);
	}

	VkCommandBuffer VulkanContext::beginOneShotCommands() const {
		VkCommandBufferAllocateInfo allocateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = mCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		vkAllocateCommandBuffers(mDevice, &allocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void VulkanContext::endOneShotCommands(VkCommandBuffer commandBuffer) const {
		vkEndCommandBuffer(commandBuffer);

		// Use a dedicated fence and wait only on this submission, instead of
		// vkQueueWaitIdle which serializes the entire graphics queue (including
		// in-flight swapchain frames) and can stall the first window present.
		VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkFence fence = VK_NULL_HANDLE;
		vkCreateFence(mDevice, &fenceInfo, nullptr, &fence);

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
		};
		vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, fence);
		vkWaitForFences(mDevice, 1, &fence, VK_TRUE, UINT64_MAX);

		vkDestroyFence(mDevice, fence, nullptr);
		vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
	}
}
