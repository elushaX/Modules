
#pragma once

#include "Scene.hpp"
#include "Color.hpp"
#include "Module.hpp"
#include "Vec.hpp"

namespace tp {

	class RayTracer {
	public:
		typedef Buffer2D<RGBA> RenderBuffer;

		struct Progress {
			halnf percentage = 0.f;
		} mProgress;

		struct OutputBuffers {
			RenderBuffer normals;
			RenderBuffer color;
			RenderBuffer depth;
			RenderBuffer albedo;
			// albedo, reflectance ...
		};

	public:
		RayTracer() = default;

		void render(const Scene& scene, OutputBuffers& out, const RenderSettings& settings);

	private:
		struct RayCastData {
			const Object* obj = nullptr;
			TrigCache* trig = nullptr;
			Vec3F hitPos = { 0, 0, 0 };
			bool hit = false;
			bool inv = false;
		};

		struct LightData {
			halnf intensity = 0;
		};

	private:
		void castRay(const Ray& ray, RayCastData& out, alnf far);
		void cycle(const RayCastData& ray, LightData& out, uhalni depth);

	private:
		RenderSettings mSettings;
		const Scene* mScene = nullptr;
	};
}
