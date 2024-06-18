#pragma once

#include "Topology.hpp"
#include "Buffer2D.hpp"
#include "Camera.hpp"

#include <string>

namespace tp {
	class GPUBuffers {
	public:
		GPUBuffers() = default;
		virtual ~GPUBuffers() = default;

		virtual void drawCall() = 0;
	};

	class Object {
	public:
		Object() = default;

		~Object() {
			delete mGUPBuffers;
		};

	public:
		Topology mTopology;
		TopologyCache mCache;
		GPUBuffers* mGUPBuffers = nullptr;
	};

	struct PointLight {
		Vec3F pos;
		halnf fallOut = 1.f;
		halnf intensity = 1.f;
	};

	class Scene {
	public:
		Scene() = default;

		bool load(const std::string& scenePath);

	public:
		Buffer<Object> mObjects;
		Buffer<PointLight> mLights;
		Camera mCamera;
	};
}
