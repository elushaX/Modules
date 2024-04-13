#pragma once

#include "Topology.hpp"
#include <memory>

class ObjectBuffers;

namespace tp {

	class Object {
	public:
		Object() = default;

	public:
		Topology mTopology;
		std::shared_ptr<ObjectBuffers> mBuffers;
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
}