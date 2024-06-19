#pragma once

#include "RayTracer.hpp"
#include "RasterRender.hpp"

namespace tp {
	class Editor {
	public:
		enum class RenderType { RASTER, PATH_TRACER };

	public:
		Editor();
		~Editor();

		void loadDefaults();

		uint4 getViewportTexID();

		void setViewportSize(const Vec2F& size);
		void renderViewport();

		void renderPathFrame();
		void setRenderType(RenderType type);
		void denoisePathRenderBuffers();

		void navigationOrbit(const Vec2F& delta);
		void navigationPan(const Vec2F& pos, const Vec2F& prevPos);
		void navigationZoom(halnf factor);
		void navigationReset();


	private:
		void sendBuffersToGPU();
		void denoise();

	private:
		Scene mScene;

		RenderType mRenderType = RenderType::RASTER;

		RasterRender mRasterRenderer;
		RayTracer mPathRenderer;

		RayTracer::OutputBuffers mPathTracerBuffers;
		uint4 mPathRenderTexture = 0;
	};
}