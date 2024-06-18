#pragma once

#include "Scene.hpp"
#include "Rect.hpp"

#include "FrameBuffer.hpp"
#include "Shader.hpp"

namespace tp {
	class RasterRender {
	public:
		explicit RasterRender(Vec2F renderResolution);
		~RasterRender();

		void render(const Scene& geometry, Vec2F size);
		uint4 getRenderBufferID();
		RenderBuffer* getRenderBuffer();
		Vec2F getBufferSize();

	private:
		RenderBuffer mRenderBuffer;
		RenderShader mDefaultShader;
	};
}