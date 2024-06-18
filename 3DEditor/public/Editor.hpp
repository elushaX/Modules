#pragma once

#include "RayTracer.hpp"
#include "RasterRender.hpp"

namespace tp {
	class Editor {
	public:
		Editor() = default;

		void loadDefaults();

		uint4 getViewportTexID();
		void setViewportSize(const Vec2F& size);
		void renderViewport();

	private:
		Scene mScene;

		RasterRender mRasterRenderer;
		RayTracer mPathRenderer;

		// TODO move inside the scene
		Vec2F mViewportSize;
	};
}