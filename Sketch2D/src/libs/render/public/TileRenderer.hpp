#pragma once

#include "CanvasWidget.hpp"
#include "CanvasFrameGeometry.hpp"
#include "Checkerboard.hpp"
#include "Compositor.hpp"
#include "Document.hpp"
#include "List.hpp"
#include "Map.hpp"
#include "FrameBuffer.hpp"

#include <vulkan/vulkan.h>

#include <array>

namespace tp {
	class VulkanContext;
}

namespace tp::sketch2d {

	enum class TileDebugState { Done, Uploading, NotUploaded, Preview, Queued, Working, Stale, Missing, Count };

	class TileRenderer {
	public:
		explicit TileRenderer(VulkanContext* context);
		~TileRenderer();

		TileRenderer(const TileRenderer&) = delete;
		TileRenderer& operator=(const TileRenderer&) = delete;

		void setViewportSize(const Vec2F& size);

		void render(Document& document, const ViewSettings& settings);

		[[nodiscard]] uint4 textureId() const;

		void setDebug(bool on) { mDebug = on; }
		[[nodiscard]] bool debug() const { return mDebug; }

		[[nodiscard]] ualni uploadedPixelsLastFrame() const { return mLastUploadedPixels; }
		[[nodiscard]] ualni uploadedTilesLastFrame() const { return mLastUploadedTiles; }
		[[nodiscard]] ualni drawnTilesLastFrame() const { return mLastDrawnTiles; }
		[[nodiscard]] ualni compositedTilesLastFrame() const { return mCompositor.lastCompositedTiles(); }
		[[nodiscard]] int4 mipLevelLastFrame() const { return mLastLevel; }
		[[nodiscard]] std::string debugLegend() const;
		[[nodiscard]] ualni debugCount(TileDebugState state) const { return mDebugCounts[(int4) state]; }
		[[nodiscard]] static RGBA debugColor(TileDebugState state);
		static constexpr ualni kDebugGridLimit = 20000;
		[[nodiscard]] int4 wantedLevelLastFrame() const { return mWantedLevel; }
		[[nodiscard]] ualni fallbackTilesLastFrame() const { return mLastFallbackTiles; }
		static constexpr ualni kUploadPixelBudget = 200000;
		static constexpr int4 kCoarseHoldFrames = 5;
		[[nodiscard]] bool hasPendingUploads() const { return mPendingUploads; }

		static constexpr ualni kDefaultUploadTilesPerFrame = 64;
		void setUploadTilesPerFrame(ualni tiles) { mUploadTilesPerFrame = tiles > 0 ? tiles : 1; }
		[[nodiscard]] ualni uploadTilesPerFrame() const { return mUploadTilesPerFrame; }

	private:
		struct GpuTile {
			VkImage image = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkDescriptorSet descriptor = VK_NULL_HANDLE;
			bool initialized = false;
			ualni frame = 0;
		};

		void createPipeline(VkRenderPass renderPass);
		void createSolidPipeline(VkRenderPass renderPass);
		void createSampler();
		void createDescriptorPool();

		GpuTile* gpuTileFor(Tile* tile);
		void destroyGpuTile(GpuTile* gpu);
		void releaseUnusedGpuTiles();
		void releaseGpuTile(const Tile& tile);
		bool uploadDirty(Tile* tile, GpuTile* gpu, VkCommandBuffer commands);
		void ensureStaging(VkDeviceSize bytes);
		void destroyStaging();
		[[nodiscard]] bool
		isTileVisible(const Document& document, const TileCoord& coord, const Vec2F& bufferSize, int4 level) const;
		template <typename Fn>
		void forEachDrawnTile(const Document& document, const Vec2F& bufferSize, int4 level, Fn&& fn);
		[[nodiscard]] bool isReady(const Tile* tile) const;
		[[nodiscard]] ualni pendingPixels(const Document& document, const Vec2F& bufferSize, int4 level);
		void uploadLevel(const Document& document, const Vec2F& bufferSize, int4 level, VkCommandBuffer commands);

		void
		drawSolidRect(void* commandBuffer, const Vec2F& pos, const Vec2F& size, const Vec2F& viewport, const RGBA& color);
		void drawDebugOverlay(Document& document, void* commandBuffer, const Vec2F& bufferSize);
		[[nodiscard]] TileDebugState debugStateOf(
			const Document& document,
			const TileCoord& coord,
			Tile* tile,
			int4 level,
			const Map<ualni, bool>& running,
			const Map<ualni, bool>& queued
		) const;
		void drawCanvasBackground(
			const Document& document, const ViewSettings& settings, void* commandBuffer, const Vec2F& bufferSize
		);

		VulkanContext* mContext = nullptr;
		RenderBuffer mBuffer;
		std::array<halnf, 2> mRotation = { 1, 0 };

		VkDescriptorSetLayout mDescriptorLayout = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkPipeline mPipeline = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE;
		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

		VkPipelineLayout mSolidPipelineLayout = VK_NULL_HANDLE;
		VkPipeline mSolidPipeline = VK_NULL_HANDLE;

		VkBuffer mStaging = VK_NULL_HANDLE;
		VkDeviceMemory mStagingMemory = VK_NULL_HANDLE;
		uint1* mStagingMapped = nullptr;
		VkDeviceSize mStagingSize = 0;
		VkDeviceSize mStagingUsed = 0;
		VkDeviceSize mStagingBudget = 0;
		bool mPendingUploads = false;
		ualni mUploadTilesPerFrame = kDefaultUploadTilesPerFrame;

		struct RetiredGpuTile {
			GpuTile* gpu;
			ualni frame;
		};

		Map<ualni, GpuTile*> mGpuTiles;
		std::vector<RetiredGpuTile> mRetiredGpuTiles;
		ualni mNextGpuId = 1;
		ualni mFrame = 0;
		ParallelFor mParallel;
		Compositor mCompositor;
		int4 mLastLevel = 0;
		ualni mDebugCounts[(int4) TileDebugState::Count] = {};
		int4 mWantedLevel = 0;
		int4 mCoarseHold = 0;
		ualni mLastFallbackTiles = 0;

		bool mDebug = false;
		ualni mLastUploadedPixels = 0;
		ualni mLastUploadedTiles = 0;
		ualni mLastDrawnTiles = 0;
	};
}
