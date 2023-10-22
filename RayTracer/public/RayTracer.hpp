
#pragma once

#include "Buffer2D.hpp"
#include "Camera.hpp"
#include "Color.hpp"
#include "Module.hpp"
#include "Topology.hpp"
#include "Vec.hpp"

namespace tp {

	extern ModuleManifest gModuleRayTracer;

	class Object {
	public:
		Object() = default;

	public:
		Topology mTopology;
		TopologyCache mCache;
	};

	struct PointLight {
		Vec3F pos;
		halnf fallOut = 1.f;
		halnf intensity = 1.f;
	};

	class Scene {
	public:
		Scene() = default;

	public:
		Buffer<Object> mObjects;
		Buffer<PointLight> mLights;
		Camera mCamera;
	};

	class RayTracer {
	public:
		typedef Buffer2D<RGBA> RenderBuffer;

		struct Progress {
			halnf percentage = 0.f;
		} mProgress;

		struct RenderSettings {
			uhalni depth = 2;
			uhalni spray = 1;
			ualni multisampling = 1;
			Vec2I size;
		};

		struct OutputBuffers {
			RenderBuffer normals;
			RenderBuffer color;
			RenderBuffer depth;
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
