#pragma once

#include "Topology.hpp"
#include "Buffer2D.hpp"
#include "Camera.hpp"

#include <string>

namespace tp {
	struct RenderSettings {
		uhalni depth = 2;
		uhalni spray = 1;
		ualni multisampling = 1;
		Vec2<ualni> size;
	};

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

		bool loadLuaFormat(const std::string& scenePath);
		bool loadOBJFormat(const std::string& objectsPath);

	public:
		Buffer<Object> mObjects;
		Buffer<PointLight> mLights;
		Camera mCamera;

		RenderSettings mRenderSettings;
	};
}
