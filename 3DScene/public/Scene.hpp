#pragma once

#include "Topology.hpp"
#include "Buffer2D.hpp"
#include "Camera.hpp"

#include <string>

struct lua_State;

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

	struct RayCastData {
		Object* obj = nullptr;
		TrigCache* trig = nullptr;
		Vec3F hitPos = { 0, 0, 0 };
		bool hit = false;
		bool inv = false;
	};

	class Scene {
		struct IOError : public std::exception {
			explicit IOError(std::string in) :
				description(std::move(in)) {}

			std::string description;
		};

	public:
		Scene() = default;

		bool load(const std::string& scenePath);

		bool loadLuaFormat(const std::string& scenePath);
		bool loadOBJFormat(const std::string& objectsPath);

		const RayCastData& castRay(const Ray& ray, alnf farVal);

		void updateCache();

	private:
		Vec3F getVec3(lua_State* state, const char* name);
		Vec2F getVec2(lua_State* state, const char* name);

		int readLight(lua_State* L, tp::PointLight* light);

	public:
		Buffer<Object> mObjects;
		Buffer<PointLight> mLights;
		Camera mCamera;

		RenderSettings mRenderSettings;

		RayCastData mRayCastData;
	};
}
