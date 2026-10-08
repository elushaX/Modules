#include "FrameBuffer.hpp"

#include "VulkanRenderTarget.hpp"
#include "VulkanContext.hpp"
#include "GpuTimer.hpp"
#include "RenderTargetRegistry.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>

namespace tp {

	class RenderBuffer::Context {
	public:
		VulkanContext* vulkan = nullptr;
		VulkanRenderTarget* target = nullptr;
		uint4 registryId = 0;
		VkCommandBuffer activeCommand = VK_NULL_HANDLE;
		bool recording = false;
		std::unique_ptr<GpuTimer> gpuTimer;
	};
}

using namespace tp;

namespace {
	tp::RenderTargetEntry entryFor(tp::VulkanRenderTarget* target, const tp::Vec2F& size) {
		return { target->resolvedColorView(), target->sampler(), (int) size.x, (int) size.y };
	}
}

RenderBuffer::RenderBuffer(const Vec2F& size) :
	mSize(size),
	mSamples(0) {
	mCtx = new Context();
	mCtx->vulkan = VulkanContext::current();
	if (!mCtx->vulkan) throw std::runtime_error("RenderBuffer: no active VulkanContext");
	mCtx->target = new VulkanRenderTarget(mCtx->vulkan, (int) size.x, (int) size.y, 0);
	mCtx->registryId = RenderTargetRegistry::registerTarget(entryFor(mCtx->target, size));
}

RenderBuffer::RenderBuffer(const Vec2F& size, tp::uint1 samples) :
	mSize(size),
	mSamples(samples) {
	mCtx = new Context();
	mCtx->vulkan = VulkanContext::current();
	if (!mCtx->vulkan) throw std::runtime_error("RenderBuffer: no active VulkanContext");
	mCtx->target = new VulkanRenderTarget(mCtx->vulkan, (int) size.x, (int) size.y, samples);
	mCtx->registryId = RenderTargetRegistry::registerTarget(entryFor(mCtx->target, size));
}

RenderBuffer::~RenderBuffer() {
	RenderTargetRegistry::unregisterTarget(mCtx->registryId);
	delete mCtx->target;
	delete mCtx;
}

void RenderBuffer::beginDraw() {
	beginCommands();
	beginPass();
}

void RenderBuffer::beginCommands() {
	if (mCtx->activeCommand) return;
	mCtx->activeCommand = mCtx->vulkan->beginOneShotCommands();
}

void RenderBuffer::beginPass() {
	beginCommands();
	VkClearColorValue clear{};
	clear.float32[0] = mClearCol.r;
	clear.float32[1] = mClearCol.g;
	clear.float32[2] = mClearCol.b;
	clear.float32[3] = mClearCol.a;

	if (mCtx->gpuTimer) mCtx->gpuTimer->begin(mCtx->activeCommand);
	mCtx->target->beginRenderPass(mCtx->activeCommand, clear);
	mCtx->recording = true;
}

void RenderBuffer::setViewport(const RectF& viewport) {
	if (!mCtx->recording) return;

	VkViewport vp{
		.x = viewport.x,
		.y = viewport.y,
		.width = viewport.z,
		.height = viewport.w,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(mCtx->activeCommand, 0, 1, &vp);
}

void RenderBuffer::clear() {}

void RenderBuffer::endDraw() {
	if (!mCtx->recording) return;

	mCtx->target->endRenderPass(mCtx->activeCommand);
	if (mCtx->gpuTimer) mCtx->gpuTimer->end(mCtx->activeCommand);
	mCtx->vulkan->endOneShotCommands(mCtx->activeCommand);
	if (mCtx->gpuTimer) mCtx->gpuTimer->collect();
	mCtx->activeCommand = VK_NULL_HANDLE;
	mCtx->recording = false;
}

void RenderBuffer::measureGpuTime(const char* name) {
	mCtx->gpuTimer = std::make_unique<GpuTimer>(
		mCtx->vulkan->device(), mCtx->vulkan->physicalDevice(), mCtx->vulkan->graphicsQueueFamily(), name
	);
}

void* RenderBuffer::activeCommandBuffer() const { return mCtx->activeCommand; }

void* RenderBuffer::renderTarget() const { return mCtx->target; }

uint4 RenderBuffer::texId() const { return mCtx->registryId; }

uint4 RenderBuffer::buffId() const { return mCtx->registryId; }

const Vec2F& RenderBuffer::getSize() const { return mSize; }

void RenderBuffer::resize(const Vec2F& size) {
	if (size == mSize) return;
	mSize = size;
	mCtx->target->resize((int) size.x, (int) size.y);
	RenderTargetRegistry::updateTarget(mCtx->registryId, entryFor(mCtx->target, size));
}
