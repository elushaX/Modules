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

		void render(const Scene& geometry, const Vec2<ualni>& size);
		uint4 getRenderBufferID();
		RenderBuffer* getRenderBuffer();
		Vec2F getBufferSize();

	private:
		RenderBuffer mRenderBuffer;
		RenderShader mDefaultShader;
	};
}