#include "TileRenderer.hpp"
#include "Profiler.hpp"

#include "VulkanContext.hpp"
#include "VulkanRenderTarget.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

	const uint32_t kTileVert[] =
#include "tile.vert.inc"
		;

	const uint32_t kTileFrag[] =
#include "tile.frag.inc"
		;

	const uint32_t kSolidVert[] =
#include "solid.vert.inc"
		;

	const uint32_t kSolidFrag[] =
#include "solid.frag.inc"
		;

	struct TilePush {
		float screenPos[2];
		float screenSize[2];
		float viewport[2];
		float rotation[2];
		float channelRows[16];
		float clip[4];
	};

	constexpr tp::sketch2d::ChannelMatrix kIdentityChannels = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	struct SolidPush {
		float rect[4];
		float viewportAndRotation[4];
		float color[4];
	};

}

namespace tp::sketch2d {

	TileRenderer::TileRenderer(VulkanContext* context) :
		mContext(context),
		mBuffer({ 16, 16 }),
		mParallel(std::max(2u, std::thread::hardware_concurrency() / 2)) {
		mCompositor.setParallel(&mParallel);
		mBuffer.mClearCol = { 0.11f, 0.11f, 0.13f, 1.0f };
		mBuffer.measureGpuTime("gpu.canvas");

		createSampler();
		createDescriptorPool();

		auto* target = static_cast<VulkanRenderTarget*>(mBuffer.renderTarget());
		createPipeline(target->renderPass());
		createSolidPipeline(target->renderPass());
	}

	TileRenderer::~TileRenderer() {
		const VkDevice device = mContext->device();
		mContext->waitIdle();

		for (auto it : mGpuTiles)
			destroyGpuTile(it->val);
		for (const RetiredGpuTile& retired : mRetiredGpuTiles)
			destroyGpuTile(retired.gpu);
		destroyStaging();

		if (mPipeline) vkDestroyPipeline(device, mPipeline, nullptr);
		if (mPipelineLayout) vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
		if (mSolidPipeline) vkDestroyPipeline(device, mSolidPipeline, nullptr);
		if (mSolidPipelineLayout) vkDestroyPipelineLayout(device, mSolidPipelineLayout, nullptr);
		if (mDescriptorLayout) vkDestroyDescriptorSetLayout(device, mDescriptorLayout, nullptr);
		if (mDescriptorPool) vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
		if (mSampler) vkDestroySampler(device, mSampler, nullptr);
	}

	void TileRenderer::createSampler() {
		VkSamplerCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.maxLod = 1.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
		};
		if (vkCreateSampler(mContext->device(), &info, nullptr, &mSampler) != VK_SUCCESS) {
			throw std::runtime_error("sketch2d: sampler create failed");
		}
	}

	void TileRenderer::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16384 };
		VkDescriptorPoolCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 16384,
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		if (vkCreateDescriptorPool(mContext->device(), &info, nullptr, &mDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("sketch2d: descriptor pool create failed");
		}

		VkDescriptorSetLayoutBinding binding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		};
		VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &binding,
		};
		if (vkCreateDescriptorSetLayout(mContext->device(), &layoutInfo, nullptr, &mDescriptorLayout) != VK_SUCCESS) {
			throw std::runtime_error("sketch2d: descriptor set layout create failed");
		}
	}

	void TileRenderer::createPipeline(VkRenderPass renderPass) {
		const VkDevice device = mContext->device();

		auto makeModule = [&](const uint32_t* code, size_t bytes) {
			VkShaderModuleCreateInfo info{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = bytes,
				.pCode = code,
			};
			VkShaderModule m = VK_NULL_HANDLE;
			if (vkCreateShaderModule(device, &info, nullptr, &m) != VK_SUCCESS) {
				throw std::runtime_error("sketch2d: shader module create failed");
			}
			return m;
		};

		VkShaderModule vert = makeModule(kTileVert, sizeof(kTileVert));
		VkShaderModule frag = makeModule(kTileFrag, sizeof(kTileFrag));

		VkPushConstantRange pushRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(TilePush),
		};
		VkPipelineLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &mDescriptorLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushRange,
		};
		if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("sketch2d: pipeline layout create failed");
		}

		const std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
			VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vert,
				.pName = "main",
			},
			VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = frag,
				.pName = "main",
			},
		};

		VkPipelineVertexInputStateCreateInfo vertexInput{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		};
		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};
		VkPipelineRasterizationStateCreateInfo rasterization{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.lineWidth = 1.0f,
		};
		VkPipelineMultisampleStateCreateInfo multisample{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};
		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_FALSE,
			.depthWriteEnable = VK_FALSE,
		};
		VkPipelineColorBlendAttachmentState blendAttachment{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask =
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
		VkPipelineColorBlendStateCreateInfo colorBlend{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blendAttachment,
		};
		const std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data(),
		};

		VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(stages.size()),
			.pStages = stages.data(),
			.pVertexInputState = &vertexInput,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterization,
			.pMultisampleState = &multisample,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlend,
			.pDynamicState = &dynamicState,
			.layout = mPipelineLayout,
			.renderPass = renderPass,
			.subpass = 0,
		};

		const VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline);

		vkDestroyShaderModule(device, vert, nullptr);
		vkDestroyShaderModule(device, frag, nullptr);

		if (result != VK_SUCCESS) throw std::runtime_error("sketch2d: tile pipeline create failed");
	}

	void TileRenderer::createSolidPipeline(VkRenderPass renderPass) {
		const VkDevice device = mContext->device();

		auto makeModule = [&](const uint32_t* code, size_t bytes) {
			VkShaderModuleCreateInfo info{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = bytes,
				.pCode = code,
			};
			VkShaderModule m = VK_NULL_HANDLE;
			if (vkCreateShaderModule(device, &info, nullptr, &m) != VK_SUCCESS) {
				throw std::runtime_error("sketch2d: solid shader module create failed");
			}
			return m;
		};

		VkShaderModule vert = makeModule(kSolidVert, sizeof(kSolidVert));
		VkShaderModule frag = makeModule(kSolidFrag, sizeof(kSolidFrag));

		VkPushConstantRange pushRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(SolidPush),
		};
		VkPipelineLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushRange,
		};
		if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &mSolidPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("sketch2d: solid pipeline layout create failed");
		}

		const std::array<VkPipelineShaderStageCreateInfo, 2> stages = {
			VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vert,
				.pName = "main",
			},
			VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = frag,
				.pName = "main",
			},
		};

		VkPipelineVertexInputStateCreateInfo vertexInput{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		};
		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};
		VkPipelineRasterizationStateCreateInfo rasterization{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.lineWidth = 1.0f,
		};
		VkPipelineMultisampleStateCreateInfo multisample{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};
		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		};
		VkPipelineColorBlendAttachmentState blendAttachment{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask =
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
		VkPipelineColorBlendStateCreateInfo colorBlend{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blendAttachment,
		};
		const std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data(),
		};

		VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(stages.size()),
			.pStages = stages.data(),
			.pVertexInputState = &vertexInput,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterization,
			.pMultisampleState = &multisample,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlend,
			.pDynamicState = &dynamicState,
			.layout = mSolidPipelineLayout,
			.renderPass = renderPass,
			.subpass = 0,
		};

		const VkResult result =
			vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mSolidPipeline);

		vkDestroyShaderModule(device, vert, nullptr);
		vkDestroyShaderModule(device, frag, nullptr);

		if (result != VK_SUCCESS) throw std::runtime_error("sketch2d: solid pipeline create failed");
	}

	void TileRenderer::drawSolidRect(
		void* commandBuffer, const Vec2F& pos, const Vec2F& size, const Vec2F& viewport, const RGBA& color
	) {
		SolidPush push{};
		push.rect[0] = pos.x;
		push.rect[1] = pos.y;
		push.rect[2] = size.x;
		push.rect[3] = size.y;
		push.viewportAndRotation[0] = viewport.x;
		push.viewportAndRotation[1] = viewport.y;
		push.viewportAndRotation[2] = mRotation[0];
		push.viewportAndRotation[3] = mRotation[1];
		push.color[0] = color.r;
		push.color[1] = color.g;
		push.color[2] = color.b;
		push.color[3] = color.a;

		auto cmd = static_cast<VkCommandBuffer>(commandBuffer);
		vkCmdPushConstants(
			cmd, mSolidPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SolidPush), &push
		);
		vkCmdDraw(cmd, 4, 1, 0, 0);
	}

	void TileRenderer::drawCanvasBackground(
		const Document& document, const ViewSettings& settings, void* commandBuffer, const Vec2F& bufferSize
	) {
		auto cmd = static_cast<VkCommandBuffer>(commandBuffer);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mSolidPipeline);

		const RectF area = visibleCanvasArea(document, bufferSize);
		if (!document.isInfinite() && settings.canvasShadow) {
			for (const ShadowBand& band : canvasShadow(frameInView(document)))
				drawSolidRect(cmd, band.rect.pos, band.rect.size, bufferSize, band.color);
		}
		if (area.size.x <= 0 || area.size.y <= 0) return;

		const CanvasBackground& background = document.background();
		if (settings.showChannel) {
			drawSolidRect(cmd, area.pos, area.size, bufferSize, channelBackground(settings.channel, background.rgba()));
			return;
		}
		if (!background.transparent) {
			drawSolidRect(cmd, area.pos, area.size, bufferSize, background.rgba());
			return;
		}

		const std::array<halnf, 2> rotation = mRotation;
		mRotation = { 1, 0 };
		const bool rotated = document.isRotated();
		if (!rotated) {
			VkRect2D clip{ { (int32_t) area.pos.x, (int32_t) area.pos.y },
										 { (uint32_t) std::ceil(area.size.x), (uint32_t) std::ceil(area.size.y) } };
			vkCmdSetScissor(cmd, 0, 1, &clip);
		}
		if (rotated && !document.isInfinite()) {
			mRotation = rotation;
			drawSolidRect(cmd, area.pos, area.size, bufferSize, kCheckerLight);
			const halnf cell = kCheckerCellPixels;
			const Vec2F origin = document.worldToView({ 0, 0 });
			const int4 firstColumn = (int4) std::floor((area.pos.x - origin.x) / cell);
			const int4 firstRow = (int4) std::floor((area.pos.y - origin.y) / cell);
			for (int4 row = firstRow; origin.y + (halnf) row * cell < area.pos.y + area.size.y; row++) {
				for (int4 column = firstColumn; origin.x + (halnf) column * cell < area.pos.x + area.size.x; column++) {
					if (((row + column) % 2 + 2) % 2 == 0) continue;
					const halnf x0 = std::max(area.pos.x, origin.x + (halnf) column * cell);
					const halnf y0 = std::max(area.pos.y, origin.y + (halnf) row * cell);
					const halnf x1 = std::min(area.pos.x + area.size.x, origin.x + (halnf) (column + 1) * cell);
					const halnf y1 = std::min(area.pos.y + area.size.y, origin.y + (halnf) (row + 1) * cell);
					if (x1 > x0 && y1 > y0) drawSolidRect(cmd, { x0, y0 }, { x1 - x0, y1 - y0 }, bufferSize, kCheckerDark);
				}
			}
		} else {
			for (const CheckerCell& cell : checkerCells(document, bufferSize))
				drawSolidRect(cmd, cell.position, cell.size, bufferSize, cell.color);
		}
		mRotation = rotation;
		VkRect2D full{ { 0, 0 }, { (uint32_t) bufferSize.x, (uint32_t) bufferSize.y } };
		vkCmdSetScissor(cmd, 0, 1, &full);
	}

	TileDebugState TileRenderer::debugStateOf(
		const Document& document,
		const TileCoord& coord,
		Tile* tile,
		int4 level,
		const Map<ualni, bool>& running,
		const Map<ualni, bool>& queued
	) const {
		if (running.presents(coord.key()).isValid()) return TileDebugState::Working;
		if (queued.presents(coord.key()).isValid()) return TileDebugState::Queued;
		for (const auto& layer : document.layers())
			if (level > 0 && layer->levels().isPreview(level, coord)) return TileDebugState::Preview;
		for (const auto& layer : document.layers())
			if (level > 0 && layer->levels().isStale(level, coord)) return TileDebugState::Stale;
		if (!tile) return TileDebugState::Missing;
		auto found = mGpuTiles.presents(tile->gpuId());
		if (tile->gpuId() == 0 || !found.isValid() || !mGpuTiles.getSlotVal(found)->initialized)
			return TileDebugState::NotUploaded;
		if (tile->hasDirty()) return TileDebugState::Uploading;
		return TileDebugState::Done;
	}

	RGBA TileRenderer::debugColor(TileDebugState state) {
		switch (state) {
			case TileDebugState::Working: return { 1.0f, 0.35f, 0.1f, 0.35f };
			case TileDebugState::Queued: return { 1.0f, 0.85f, 0.1f, 0.30f };
			case TileDebugState::Preview: return { 0.75f, 0.3f, 1.0f, 0.30f };
			case TileDebugState::Uploading: return { 0.2f, 0.6f, 1.0f, 0.30f };
			case TileDebugState::NotUploaded: return { 1.0f, 0.1f, 0.2f, 0.35f };
			case TileDebugState::Stale: return { 0.95f, 0.45f, 0.75f, 0.30f };
			case TileDebugState::Missing: return { 0.5f, 0.5f, 0.5f, 0.15f };
			case TileDebugState::Done: return { 0.2f, 0.9f, 0.4f, 0.12f };
		}
		return { 1, 1, 1, 0.1f };
	}

	void TileRenderer::drawDebugOverlay(Document& document, void* commandBuffer, const Vec2F& bufferSize) {
		auto cmd = static_cast<VkCommandBuffer>(commandBuffer);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mSolidPipeline);
		mRotation = { std::cos(document.rotation()), std::sin(document.rotation()) };

		const int4 level = mLastLevel;
		const int4 levelSpan = kTileSize << level;
		const halnf screenTile = (halnf) levelSpan * document.zoom();
		const halnf line = 1.0f;

		Map<ualni, bool> running;
		Map<ualni, bool> queued;
		document.engine().forEachPending([&](const Layer&, const TileWork& work, bool active) {
			const TileCoord at = { floordiv(work.coord.x, 1 << std::max(0, level - work.level)),
														 floordiv(work.coord.y, 1 << std::max(0, level - work.level)) };
			(active ? running : queued).put(at.key(), true);
		});

		for (int4 index = 0; index < (int4) TileDebugState::Count; index++)
			mDebugCounts[index] = 0;

		const auto outline = [&](Vec2F view, const RGBA& color) {
			drawSolidRect(cmd, view, { screenTile, line }, bufferSize, color);
			drawSolidRect(cmd, { view.x, view.y + screenTile - line }, { screenTile, line }, bufferSize, color);
			drawSolidRect(cmd, view, { line, screenTile }, bufferSize, color);
			drawSolidRect(cmd, { view.x + screenTile - line, view.y }, { line, screenTile }, bufferSize, color);
		};

		const RectF bounds = document.viewBoundsOfScreen(bufferSize);
		const Vec2F topLeft = document.panWorld() + bounds.pos / document.zoom();
		const Vec2F bottomRight = document.panWorld() + (bounds.pos + bounds.size) / document.zoom();
		const int4 x0 = (int4) std::floor(std::min(topLeft.x, bottomRight.x) / (halnf) levelSpan);
		const int4 y0 = (int4) std::floor(std::min(topLeft.y, bottomRight.y) / (halnf) levelSpan);
		const int4 x1 = (int4) std::floor(std::max(topLeft.x, bottomRight.x) / (halnf) levelSpan);
		const int4 y1 = (int4) std::floor(std::max(topLeft.y, bottomRight.y) / (halnf) levelSpan);
		if ((ualni) (x1 - x0 + 1) * (ualni) (y1 - y0 + 1) > kDebugGridLimit) return;

		for (int4 y = y0; y <= y1; y++) {
			for (int4 x = x0; x <= x1; x++) {
				const TileCoord coord = { x, y };
				if (!isTileVisible(document, coord, bufferSize, level)) continue;
				const TileDebugState state =
					debugStateOf(document, coord, mCompositor.tileAt(level, coord), level, running, queued);
				mDebugCounts[(int4) state]++;
				const Vec2F view = document.worldToView({ (halnf) (x * levelSpan), (halnf) (y * levelSpan) });
				const RGBA color = debugColor(state);
				if (state != TileDebugState::Missing) drawSolidRect(cmd, view, { screenTile, screenTile }, bufferSize, color);
				outline(view, { color.r, color.g, color.b, std::min(1.0f, color.a * 2.5f) });
			}
		}
		mRotation = { 1, 0 };
	}

	std::string TileRenderer::debugLegend() const {
		static const char* const kNames[] = { "done",   "uploading", "not uploaded", "preview",
																					"queued", "working",   "stale",        "empty" };
		std::string text = "L" + std::to_string(mLastLevel) + "  ";
		for (int4 index = 0; index < (int4) TileDebugState::Count; index++)
			text += std::string(kNames[index]) + " " + std::to_string(mDebugCounts[index]) + "  ";
		return text;
	}

	void TileRenderer::destroyGpuTile(GpuTile* gpu) {
		const VkDevice device = mContext->device();
		if (gpu->descriptor) vkFreeDescriptorSets(device, mDescriptorPool, 1, &gpu->descriptor);
		if (gpu->view) vkDestroyImageView(device, gpu->view, nullptr);
		if (gpu->image) vkDestroyImage(device, gpu->image, nullptr);
		if (gpu->memory) vkFreeMemory(device, gpu->memory, nullptr);
		delete gpu;
	}

	void TileRenderer::releaseGpuTile(const Tile& tile) {
		if (tile.gpuId() == 0) return;
		auto found = mGpuTiles.presents(tile.gpuId());
		if (!found.isValid()) return;
		mRetiredGpuTiles.push_back({ mGpuTiles.getSlotVal(found), mFrame });
		mGpuTiles.remove(tile.gpuId());
	}

	void TileRenderer::releaseUnusedGpuTiles() {
		for (const std::shared_ptr<Tile>& tile : mCompositor.lastRemoved())
			if (tile.use_count() == 1) releaseGpuTile(*tile);

		size_t kept = 0;
		for (const RetiredGpuTile& retired : mRetiredGpuTiles) {
			if (retired.frame + 2 < mFrame) destroyGpuTile(retired.gpu);
			else mRetiredGpuTiles[kept++] = retired;
		}
		mRetiredGpuTiles.resize(kept);
	}

	TileRenderer::GpuTile* TileRenderer::gpuTileFor(Tile* tile) {
		if (tile->gpuId() != 0) {
			auto idx = mGpuTiles.presents(tile->gpuId());
			if (idx.isValid()) return mGpuTiles.getSlotVal(idx);
		}

		const VkDevice device = mContext->device();
		auto* gpu = new GpuTile();

		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = { (uint32_t) kTileSize, (uint32_t) kTileSize, 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		vkCreateImage(device, &imageInfo, nullptr, &gpu->image);

		VkMemoryRequirements req{};
		vkGetImageMemoryRequirements(device, gpu->image, &req);
		VkMemoryAllocateInfo alloc{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = req.size,
			.memoryTypeIndex = mContext->findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		vkAllocateMemory(device, &alloc, nullptr, &gpu->memory);
		vkBindImageMemory(device, gpu->image, gpu->memory, 0);

		VkImageViewCreateInfo viewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = gpu->image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		};
		vkCreateImageView(device, &viewInfo, nullptr, &gpu->view);

		VkDescriptorSetAllocateInfo descAlloc{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = mDescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &mDescriptorLayout,
		};
		vkAllocateDescriptorSets(device, &descAlloc, &gpu->descriptor);

		VkDescriptorImageInfo descImage{
			.sampler = mSampler,
			.imageView = gpu->view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		VkWriteDescriptorSet write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = gpu->descriptor,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &descImage,
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

		const ualni id = mNextGpuId++;
		tile->setGpuId(id);
		mGpuTiles.put(id, gpu);

		tile->markDirty({ 0, 0, (halnf) kTileSize, (halnf) kTileSize });
		return gpu;
	}

	void TileRenderer::ensureStaging(VkDeviceSize bytes) {
		if (bytes <= mStagingSize) return;
		destroyStaging();

		const VkDevice device = mContext->device();
		const VkDeviceSize size = std::max<VkDeviceSize>(bytes, (VkDeviceSize) kTileSize * kTileSize * 4 * 16);
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};
		vkCreateBuffer(device, &bufferInfo, nullptr, &mStaging);
		VkMemoryRequirements req{};
		vkGetBufferMemoryRequirements(device, mStaging, &req);
		VkMemoryAllocateInfo alloc{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = req.size,
			.memoryTypeIndex = mContext->findMemoryType(
				req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			),
		};
		vkAllocateMemory(device, &alloc, nullptr, &mStagingMemory);
		vkBindBufferMemory(device, mStaging, mStagingMemory, 0);
		void* mapped = nullptr;
		vkMapMemory(device, mStagingMemory, 0, size, 0, &mapped);
		mStagingMapped = static_cast<uint1*>(mapped);
		mStagingSize = size;
	}

	void TileRenderer::destroyStaging() {
		if (!mStaging) return;
		const VkDevice device = mContext->device();
		vkUnmapMemory(device, mStagingMemory);
		vkDestroyBuffer(device, mStaging, nullptr);
		vkFreeMemory(device, mStagingMemory, nullptr);
		mStaging = VK_NULL_HANDLE;
		mStagingMemory = VK_NULL_HANDLE;
		mStagingMapped = nullptr;
		mStagingSize = 0;
	}

	bool TileRenderer::isTileVisible(
		const Document& document, const TileCoord& coord, const Vec2F& bufferSize, int4 level
	) const {
		const int4 levelTiles = 1 << level;
		const halnf screenTile = (halnf) (kTileSize * levelTiles) * document.zoom();
		const Vec2F view =
			document.worldToView({ (halnf) (coord.x * kTileSize * levelTiles), (halnf) (coord.y * kTileSize * levelTiles) });
		const RectF bounds = document.viewBoundsOfScreen(bufferSize);
		return view.x + screenTile >= bounds.pos.x && view.y + screenTile >= bounds.pos.y &&
					 view.x <= bounds.pos.x + bounds.size.x && view.y <= bounds.pos.y + bounds.size.y;
	}

	bool TileRenderer::uploadDirty(Tile* tile, GpuTile* gpu, VkCommandBuffer cmd) {
		if (!tile->hasDirty() && gpu->initialized) return true;

		const RectF dirty = tile->hasDirty() ? tile->dirtyRect() : RectF{ 0, 0, (halnf) kTileSize, (halnf) kTileSize };
		const int4 dx = gpu->initialized ? (int4) dirty.pos.x : 0;
		const int4 dy = gpu->initialized ? (int4) dirty.pos.y : 0;
		const int4 dw = gpu->initialized ? (int4) dirty.size.x : kTileSize;
		const int4 dh = gpu->initialized ? (int4) dirty.size.y : kTileSize;

		const VkDeviceSize uploadBytes = (VkDeviceSize) dw * dh * 4;
		if (mStagingUsed + uploadBytes > mStagingBudget) return false;

		const VkDeviceSize offset = mStagingUsed;
		mStagingUsed += (uploadBytes + 15) & ~(VkDeviceSize) 15;
		uint1* dst = mStagingMapped + offset;
		const uint1* src = tile->pixels();
		for (int4 row = 0; row < dh; row++) {
			std::memcpy(dst + (ualni) row * dw * 4, src + ((ualni) (dy + row) * kTileSize + dx) * 4, (size_t) dw * 4);
		}

		VkImageMemoryBarrier toDst{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = gpu->initialized ? VK_ACCESS_SHADER_READ_BIT : (VkAccessFlags) 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = gpu->initialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = gpu->image,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		};
		vkCmdPipelineBarrier(
			cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toDst
		);

		VkBufferImageCopy region{
			.bufferOffset = offset,
			.bufferRowLength = (uint32_t) dw,
			.bufferImageHeight = (uint32_t) dh,
			.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.imageOffset = { dx, dy, 0 },
			.imageExtent = { (uint32_t) dw, (uint32_t) dh, 1 },
		};
		vkCmdCopyBufferToImage(cmd, mStaging, gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier toRead = toDst;
		toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(
			cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toRead
		);

		gpu->initialized = true;
		mLastUploadedPixels += (ualni) dw * dh;
		mLastUploadedTiles++;
		tile->clearDirty();
		return true;
	}

	template <typename Fn>
	void TileRenderer::forEachDrawnTile(const Document& document, const Vec2F& bufferSize, int4 level, Fn&& fn) {
		const int4 levelTiles = 1 << level;
		const halnf span = (halnf) (kTileSize * levelTiles);
		const RectF bounds = document.viewBoundsOfScreen(bufferSize);
		const Vec2F topLeft = document.panWorld() + bounds.pos / document.zoom();
		const Vec2F bottomRight = document.panWorld() + (bounds.pos + bounds.size) / document.zoom();
		const int4 x0 = (int4) std::floor(std::min(topLeft.x, bottomRight.x) / span);
		const int4 y0 = (int4) std::floor(std::min(topLeft.y, bottomRight.y) / span);
		const int4 x1 = (int4) std::floor(std::max(topLeft.x, bottomRight.x) / span);
		const int4 y1 = (int4) std::floor(std::max(topLeft.y, bottomRight.y) / span);
		const ualni visible = (ualni) (x1 - x0 + 1) * (ualni) (y1 - y0 + 1);
		const ualni stored = mCompositor.tileCount(level);

		if (visible > stored) {
			const auto inView = [&](const TileCoord& coord, Tile* tile) {
				if (coord.x >= x0 && coord.x <= x1 && coord.y >= y0 && coord.y <= y1) fn(coord, tile);
			};
			mCompositor.forEachTile(level, inView);
			return;
		}
		for (int4 y = y0; y <= y1; y++)
			for (int4 x = x0; x <= x1; x++) {
				Tile* tile = mCompositor.tileAt(level, { x, y });
				if (tile) fn(TileCoord{ x, y }, tile);
			}
	}

	void TileRenderer::setViewportSize(const Vec2F& size) {
		if (size.x <= 0 || size.y <= 0) return;
		mBuffer.resize(size);
	}

	uint4 TileRenderer::textureId() const { return mBuffer.texId(); }

	bool TileRenderer::isReady(const Tile* tile) const {
		if (!tile || tile->gpuId() == 0) return false;
		auto found = mGpuTiles.presents(tile->gpuId());
		return found.isValid() && mGpuTiles.getSlotVal(found)->initialized;
	}

	ualni TileRenderer::pendingPixels(const Document& document, const Vec2F& bufferSize, int4 level) {
		ualni pending = 0;
		forEachDrawnTile(document, bufferSize, level, [&](const TileCoord& coord, Tile* tile) {
			if (!isTileVisible(document, coord, bufferSize, level)) return;
			if (!isReady(tile)) pending += (ualni) kTileSize * kTileSize;
			else if (tile->hasDirty()) pending += (ualni) (tile->dirtyRect().size.x * tile->dirtyRect().size.y);
		});
		return pending;
	}

	void
	TileRenderer::uploadLevel(const Document& document, const Vec2F& bufferSize, int4 level, VkCommandBuffer commands) {
		forEachDrawnTile(document, bufferSize, level, [&](const TileCoord& coord, Tile* tile) {
			if (!isTileVisible(document, coord, bufferSize, level)) return;
			GpuTile* gpu = gpuTileFor(tile);
			gpu->frame = mFrame;
			if (!uploadDirty(tile, gpu, commands)) mPendingUploads = true;
		});
	}

	void TileRenderer::render(Document& document, const ViewSettings& settings) {
		mLastUploadedPixels = 0;
		mLastUploadedTiles = 0;
		mLastDrawnTiles = 0;
		mLastFallbackTiles = 0;

		TP_PROFILE_SCOPE("canvas.render");
		mFrame++;
		const Vec2F bufferSize = mBuffer.getSize();
		mWantedLevel = LayerLevels::levelForZoom(document.zoom());
		const int4 coarser = std::min(mWantedLevel + 1, LayerLevels::kLevels - 1);
		{
			TP_PROFILE_SCOPE("canvas.composite");
			document.whilePaintPaused([&]() {
				if (coarser != mWantedLevel) mCompositor.update(document, coarser);
				mCompositor.update(document, mWantedLevel);
			});
			TP_PROFILE_COUNT("canvas.composited_pixels", mCompositor.lastCompositedPixels());
		}
		if (coarser != mWantedLevel && pendingPixels(document, bufferSize, mWantedLevel) > kUploadPixelBudget)
			mCoarseHold = kCoarseHoldFrames;
		mLastLevel = mCoarseHold > 0 ? coarser : mWantedLevel;
		if (mCoarseHold > 0) mCoarseHold--;

		mBuffer.beginCommands();
		VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(mBuffer.activeCommandBuffer());
		{
			TP_PROFILE_SCOPE("canvas.upload");
			const VkDeviceSize tileBytes = (VkDeviceSize) kTileSize * kTileSize * 4;
			const VkDeviceSize budget = std::min<VkDeviceSize>(
				tileBytes * (VkDeviceSize) mUploadTilesPerFrame,
				std::max<VkDeviceSize>((VkDeviceSize) kUploadPixelBudget * 4, tileBytes)
			);
			ensureStaging(budget);
			mStagingUsed = 0;
			mStagingBudget = budget;
			mPendingUploads = false;
			uploadLevel(document, bufferSize, coarser, commandBuffer);
			if (coarser != mWantedLevel) uploadLevel(document, bufferSize, mWantedLevel, commandBuffer);
			releaseUnusedGpuTiles();
		}
		TP_PROFILE_SCOPE("canvas.record");

		const halnf zoom = document.zoom();

		mBuffer.mClearCol = settings.boardColor.rgba();
		mBuffer.beginPass();

		mBuffer.setViewport({ 0.0f, 0.0f, bufferSize.x, bufferSize.y });
		VkRect2D scissor{ { 0, 0 }, { (uint32_t) bufferSize.x, (uint32_t) bufferSize.y } };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		mRotation = { std::cos(document.rotation()), std::sin(document.rotation()) };

		drawCanvasBackground(document, settings, commandBuffer, bufferSize);

		const ChannelMatrix channelRows = settings.showChannel ? channelMatrix(settings.channel) : kIdentityChannels;
		const RectF clip = document.isInfinite() ? RectF{ -1.0e9f, -1.0e9f, 2.0e9f, 2.0e9f } : frameInView(document);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		const auto drawTile = [&](const TileCoord& coord, Tile* tile, int4 level, const RectF& area) {
			const int4 levelTiles = 1 << level;
			const halnf screenTile = (halnf) (kTileSize * levelTiles) * zoom;
			const Vec2F worldTopLeft = { (halnf) (coord.x * kTileSize * levelTiles),
																	 (halnf) (coord.y * kTileSize * levelTiles) };
			const Vec2F screen = document.worldToView(worldTopLeft);

			auto found = mGpuTiles.presents(tile->gpuId());
			if (tile->gpuId() == 0 || !found.isValid()) return false;
			GpuTile* gpu = mGpuTiles.getSlotVal(found);
			if (!gpu->initialized) return false;
			const halnf left = std::max(clip.pos.x, area.pos.x);
			const halnf top = std::max(clip.pos.y, area.pos.y);
			const halnf right = std::min(clip.pos.x + clip.size.x, area.pos.x + area.size.x);
			const halnf bottom = std::min(clip.pos.y + clip.size.y, area.pos.y + area.size.y);

			TilePush push{};
			push.screenPos[0] = screen.x;
			push.screenPos[1] = screen.y;
			push.screenSize[0] = screenTile;
			push.screenSize[1] = screenTile;
			push.viewport[0] = bufferSize.x;
			push.viewport[1] = bufferSize.y;
			push.rotation[0] = mRotation[0];
			push.rotation[1] = mRotation[1];
			push.clip[0] = left;
			push.clip[1] = top;
			push.clip[2] = right;
			push.clip[3] = bottom;

			std::copy(channelRows.begin(), channelRows.end(), push.channelRows);
			vkCmdPushConstants(
				commandBuffer,
				mPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TilePush),
				&push
			);
			vkCmdBindDescriptorSets(
				commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &gpu->descriptor, 0, nullptr
			);
			vkCmdDraw(commandBuffer, 4, 1, 0, 0);
			return true;
		};

		const RectF everywhere = { -1.0e9f, -1.0e9f, 2.0e9f, 2.0e9f };
		const int4 drawLevel = mLastLevel;
		const int4 parentLevel = std::min(drawLevel + 1, LayerLevels::kLevels - 1);
		forEachDrawnTile(document, bufferSize, drawLevel, [&](const TileCoord& coord, Tile* tile) {
			if (!isTileVisible(document, coord, bufferSize, drawLevel)) return;
			if (drawTile(coord, tile, drawLevel, everywhere)) {
				mLastDrawnTiles++;
				return;
			}
			if (parentLevel == drawLevel) return;
			const TileCoord parent = LayerLevels::parentOf(coord);
			Tile* coarse = mCompositor.tileAt(parentLevel, parent);
			if (!coarse) return;
			const halnf span = (halnf) (kTileSize << drawLevel) * zoom;
			const Vec2F origin = document.worldToView(
				{ (halnf) (coord.x * (kTileSize << drawLevel)), (halnf) (coord.y * (kTileSize << drawLevel)) }
			);
			if (drawTile(parent, coarse, parentLevel, { origin.x, origin.y, span, span })) mLastFallbackTiles++;
		});

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		if (mDebug) {
			drawDebugOverlay(document, commandBuffer, bufferSize);
		}

		mBuffer.endDraw();
		TP_PROFILE_COUNT("canvas.tiles_drawn", mLastDrawnTiles);
		TP_PROFILE_COUNT("canvas.tiles_uploaded", mLastUploadedTiles);
		TP_PROFILE_COUNT("canvas.pixels_uploaded", mLastUploadedPixels);
		TP_PROFILE_COUNT("canvas.mip_level", mLastLevel);
	}
}
