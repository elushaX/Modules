#include "RasterRender.hpp"

#include "GPUBuffers.hpp"
#include "VulkanObjectPipeline.hpp"
#include "VulkanRenderTarget.hpp"
#include "VulkanClipSpace.hpp"
#include "VulkanContext.hpp"

namespace tp {

	class RasterRender::Context {
	public:
		VulkanContext* vulkan = nullptr;
		VulkanObjectPipeline* defaultPipeline = nullptr;
		VulkanObjectPipeline* solidPipeline = nullptr;
		VkRenderPass builtForRenderPass = VK_NULL_HANDLE;

		~Context() {
			delete defaultPipeline;
			delete solidPipeline;
		}

		void ensurePipelines(VulkanRenderTarget* target) {
			if (defaultPipeline && builtForRenderPass == target->renderPass()) return;

			delete defaultPipeline;
			delete solidPipeline;

			defaultPipeline = new VulkanObjectPipeline(vulkan, target->renderPass(), target->sampleCount(), false);
			solidPipeline = new VulkanObjectPipeline(vulkan, target->renderPass(), target->sampleCount(), true);
			builtForRenderPass = target->renderPass();
		}
	};

	namespace {

		ObjectPushConstants makePushConstants(const Mat4F& cameraMat, const Object& object) {
			Mat4F camera = vulkanCameraMatrix(cameraMat);
			Mat4F basisMat = toMat4(object.mTopology.Basis);
			Vec4F originPoint = toVec4(object.mTopology.Origin);

			Mat4F mvp = camera * basisMat;

			ObjectPushConstants constants;
			constants.mvp = mvp.transpose();
			constants.offset = camera * originPoint;
			return constants;
		}
	}

	RasterRender::RasterRender() {
		mCtx = new Context();
		mCtx->vulkan = VulkanContext::current();
	}

	RasterRender::~RasterRender() { delete mCtx; }

	void RasterRender::resize(const Vec2<tp::ualni>& size) {
		auto sizeF = Vec2F(size.x, size.y);
		mRenderBuffer.resize(sizeF);
		mTempBuffer.resize(sizeF);
	}

	uint4 RasterRender::getRenderBufferID() { return mRenderBuffer.texId(); }

	Vec2F RasterRender::getBufferSize() { return mRenderBuffer.getSize(); }

	void RasterRender::renderDefault(const Scene& geometry) {
		mRenderBuffer.mClearCol = { 0.0f, 0.0f, 0.0f, 0.0f };

		mRenderBuffer.beginDraw();

		auto target = static_cast<VulkanRenderTarget*>(mRenderBuffer.renderTarget());
		auto cmd = static_cast<VkCommandBuffer>(mRenderBuffer.activeCommandBuffer());
		mCtx->ensurePipelines(target);

		mCtx->defaultPipeline->bind(cmd);

		Mat4F cameraMat = geometry.mCamera.calculateTransformationMatrix();

		for (auto object : geometry.mObjects) {
			mCtx->defaultPipeline->pushUniforms(cmd, makePushConstants(cameraMat, object.data()));
			object->mGUPBuffers->drawCall(cmd);
		}

		mRenderBuffer.endDraw();
	}

	void RasterRender::renderOutline(const Camera& camera, const Object& object) {
		mRenderBuffer.beginDraw();

		auto target = static_cast<VulkanRenderTarget*>(mRenderBuffer.renderTarget());
		auto cmd = static_cast<VkCommandBuffer>(mRenderBuffer.activeCommandBuffer());
		mCtx->ensurePipelines(target);

		mCtx->solidPipeline->bind(cmd);
		mCtx->solidPipeline->pushUniforms(cmd, makePushConstants(camera.calculateTransformationMatrix(), object));
		object.mGUPBuffers->drawCall(cmd);

		mRenderBuffer.endDraw();
	}

	void RasterRender::beginRender(const Scene& geometry, const Vec2<tp::ualni>& size) {
		resize(size);

		for (auto object : geometry.mObjects) {
			if (!object->mGUPBuffers) {
				object->mGUPBuffers = new ObjectBuffers(&object.data());
			}
		}
	}

	void RasterRender::endRender() {}

	RenderBuffer* RasterRender::getRenderBuffer() { return &mRenderBuffer; }
}
