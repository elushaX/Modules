
#pragma once

#include "Rect.hpp"
#include "Color.hpp"

namespace tp {
	class RenderBuffer {
	public:
		RenderBuffer(const Vec2F& size);
		RenderBuffer(const Vec2F& size, tp::uint1 samples);
		~RenderBuffer();

		void beginDraw();
		void setViewport(const RectF& viewport);
		void clear();
		void endDraw();

		uint4 texId() const;
		uint4 buffId() const;

		const Vec2F& getSize() const;

	public:
		RGBA mClearCol = 0.f;

	private:
		uint4 mFrameBufferID = 0; // regroups 0, 1, or more textures, and 0 or 1 depth buffer.
		uint4 mTextureId = 0;     // texture we're going to render to ( colour attachement #0 )
		uint4 mDepthBufferID = 0;
		uint4 mDrawBuffers[1];
		Vec2F mSize;
	};
};