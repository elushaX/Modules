#pragma once

#include "Scene.hpp"
#include "Rect.hpp"

#include "FrameBuffer.hpp"
#include "Shader.hpp"

namespace tp {
	class RasterRender {
	public:
		RasterRender();
		~RasterRender();

		void beginRender(const Scene& geometry, const Vec2<ualni>& size);
		void endRender();

		void renderDefault(const Scene& geometry);
		void renderOutline(const Camera& camera, const Object& object);

		void resize(const Vec2<ualni>& size);

		uint4 getRenderBufferID();
		RenderBuffer* getRenderBuffer();
		Vec2F getBufferSize();

	private:
		void bindObjectShaderAttributes(const Object& object);
		void bindCameraShaderAttributes(const Mat4F& cameraMat);

	private:
		RenderBuffer mRenderBuffer;
		RenderBuffer mTempBuffer;

		RenderShader mDefaultShader;
		RenderShader mSolidShader;
	};
}