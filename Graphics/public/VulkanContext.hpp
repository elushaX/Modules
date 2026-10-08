#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace tp {

	class GpuTimer;

	class VulkanSurfaceSource {
	public:
		virtual ~VulkanSurfaceSource() = default;

		[[nodiscard]] virtual std::vector<const char*> requiredInstanceExtensions() const = 0;
		[[nodiscard]] virtual VkSurfaceKHR createSurface(VkInstance instance) const = 0;
		[[nodiscard]] virtual VkExtent2D framebufferExtent() const = 0;
		[[nodiscard]] virtual bool wantsTranslucency() const = 0;
		virtual void onTranslucencyResolved(bool translucent) = 0;
	};

	class VulkanContext {
	public:
		explicit VulkanContext(VulkanSurfaceSource& surfaceSource);
		~VulkanContext();

		VulkanContext(const VulkanContext&) = delete;
		VulkanContext& operator=(const VulkanContext&) = delete;

		VkInstance instance() const { return mInstance; }
		VkDevice device() const { return mDevice; }
		VkPhysicalDevice physicalDevice() const { return mPhysicalDevice; }
		VkQueue graphicsQueue() const { return mGraphicsQueue; }
		VkRenderPass renderPass() const { return mRenderPass; }
		uint32_t swapchainImageCount() const { return static_cast<uint32_t>(mSwapchainImages.size()); }
		VkExtent2D swapchainExtent() const { return mSwapchainExtent; }

		VkCommandBuffer* commandBufferArray() { return mCommandBuffers.data(); }
		VkCommandBuffer activeCommandBuffer() const { return mCommandBuffers[mCurrentFrame]; }
		uint32_t* currentFrameSlot() { return &mCurrentFrame; }
		uint32_t framesInFlight() const { return kFramesInFlight; }
		bool isTranslucent() const { return mTranslucent; }

		bool beginFrame(int framebufferWidth, int framebufferHeight);
		[[nodiscard]] uint64_t swapchainRecreations() const { return mSwapchainRecreations; }
		void endFrame();
		void waitIdle();

		uint32_t graphicsQueueFamily() const { return mGraphicsQueueFamily; }
		VkCommandPool commandPool() const { return mCommandPool; }
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		VkCommandBuffer beginOneShotCommands() const;
		void endOneShotCommands(VkCommandBuffer commandBuffer) const;

		void syncSwapchainToWindow();
		void releaseSurface();
		void recreateSurface();
		[[nodiscard]] bool hasSurface() const { return mSurface != VK_NULL_HANDLE; }

		static VulkanContext* current();

	private:
		uint64_t mSwapchainRecreations = 0;
		VkSurfaceTransformFlagBitsKHR mSurfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkExtent2D mSurfaceExtent{ 0, 0 };
		VkExtent2D mRequestedExtent{ 0, 0 };
		void createInstance();
		void createSurface();
		void selectCompositeAlpha();
		void selectPhysicalDevice();
		void createLogicalDevice();
		void createCommandResources();
		void createSyncObjects();
		void createRenderPass();

		bool swapchainMatches(int framebufferWidth, int framebufferHeight) const;
		VkPresentModeKHR selectPresentMode() const;
		void createSwapchain(int width, int height, VkSwapchainKHR previous = VK_NULL_HANDLE);
		void destroySwapchainResources();
		void destroySwapchain();
		void recreateSwapchain(int width, int height);
		[[nodiscard]] bool surfaceChanged() const;

		VkFormat selectDepthStencilFormat() const;
		void createDepthStencil();
		void destroyDepthStencil();

		VulkanSurfaceSource& mSurfaceSource;
		VkCompositeAlphaFlagBitsKHR mCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		bool mTranslucent = false;

		VkInstance mInstance = VK_NULL_HANDLE;
		VkSurfaceKHR mSurface = VK_NULL_HANDLE;
		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkDevice mDevice = VK_NULL_HANDLE;

		uint32_t mGraphicsQueueFamily = UINT32_MAX;
		uint32_t mPresentQueueFamily = UINT32_MAX;
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkQueue mPresentQueue = VK_NULL_HANDLE;

		VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
		VkPresentModeKHR mPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkFormat mSwapchainFormat{};
		VkExtent2D mSwapchainExtent{};
		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainImageViews;
		std::vector<VkFramebuffer> mSwapchainFramebuffers;

		VkRenderPass mRenderPass = VK_NULL_HANDLE;

		VkFormat mDepthStencilFormat{};
		VkImage mDepthStencilImage = VK_NULL_HANDLE;
		VkDeviceMemory mDepthStencilMemory = VK_NULL_HANDLE;
		VkImageView mDepthStencilView = VK_NULL_HANDLE;

		static constexpr uint32_t kFramesInFlight = 2;

		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> mCommandBuffers;

		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;

		uint32_t mCurrentFrame = 0;
		uint32_t mAcquiredImageIndex = 0;
		bool mFrameActive = false;

		std::unique_ptr<GpuTimer> mFrameGpuTimer;
	};
}
