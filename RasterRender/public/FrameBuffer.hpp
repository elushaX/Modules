
#pragma once

#include "Rect.hpp"
#include "Color.hpp"

namespace tp {
	class RenderBuffer {
	public:
		class Context;

		explicit RenderBuffer(const Vec2F& size = { 10, 10 });
		RenderBuffer(const Vec2F& size, tp::uint1 samples);
		~RenderBuffer();

		void beginDraw();
		void beginCommands();
		void beginPass();
		void setViewport(const RectF& viewport);
		void clear();
		void endDraw();
		void measureGpuTime(const char* name);

		[[nodiscard]] uint4 texId() const;
		[[nodiscard]] uint4 buffId() const;

		void resize(const Vec2F& size);

		[[nodiscard]] const Vec2F& getSize() const;

		[[nodiscard]] Context* context() const { return mCtx; }

		[[nodiscard]] void* activeCommandBuffer() const;
		[[nodiscard]] void* renderTarget() const;

	public:
		RGBA mClearCol = 0.f;

	private:
		Context* mCtx = nullptr;
		Vec2F mSize;
		ualni mSamples = 0;
	};
};