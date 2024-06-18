#pragma once

#include "Scene.hpp"
#include "Rect.hpp"

#include "FrameBuffer.hpp"
#include "Shader.hpp"

namespace tp {
	class Render {
	public:
		explicit Render(Vec2F renderResolution);
		~Render();

		void render(const Scene& geometry, Vec2F size);
		uint4 getRenderBufferID();
		RenderBuffer* getRenderBuffer();
		Vec2F getBufferSize();

	private:
		RenderBuffer mRenderBuffer;
		RenderShader mDefaultShader;
	};
}