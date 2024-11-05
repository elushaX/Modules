
#include "Scene.hpp"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#include <filesystem>
#include <utility>

tp::Vec3F tp::Scene::getVec3(lua_State* state, const char* name) {
	int stackSize = lua_gettop(state);

	// Access the "pos" field and validate it
	lua_getfield(state, -1, name);
	if (!lua_istable(state, -1) || lua_rawlen(state, -1) != 3) {
		throw IOError(std::string("Invalid vec3 named ") + name);
	}

	// Read the values from the table
	float pos[3];
	for (int i = 0; i < 3; i++) {
		lua_rawgeti(state, -1, i + 1);
		if (lua_isnumber(state, -1)) {
			pos[i] = lua_tonumber(state, -1);
		} else {
			throw IOError("vec3 values must be numbers");
		}
		lua_pop(state, 1); // pop number
	}
	lua_pop(state, 1); // pop vec3

	ASSERT(stackSize == lua_gettop(state))

	return { pos[0], pos[1], pos[2] };
}

tp::Vec2F tp::Scene::getVec2(lua_State* state, const char* name) {
	int stackSize = lua_gettop(state);

	// Access the "pos" field and validate it
	lua_getfield(state, -1, name);
	if (!lua_istable(state, -1) || lua_rawlen(state, -1) != 2) {
		throw IOError(std::string("Invalid vec2 named ") + name);
	}

	// Read the values from the table
	float pos[2];
	for (int i = 0; i < 2; i++) {
		lua_rawgeti(state, -1, i + 1);
		if (lua_isnumber(state, -1)) {
			pos[i] = lua_tonumber(state, -1);
		} else {
			throw IOError("vec3 values must be numbers");
		}
		lua_pop(state, 1); // pop number
	}
	lua_pop(state, 1); // pop vec3

	ASSERT(stackSize == lua_gettop(state))

	return { pos[0], pos[1] };
}

// Function to read a Lua table representing RenderSettings
int readRenderSettings(lua_State* L, tp::RenderSettings& settings) {
	lua_getglobal(L, "RenderSettings");
	if (!lua_istable(L, -1)) {
		printf("RenderSettings is not a table.\n");
		return 0; // Error
	}

	// Read depth field
	lua_getfield(L, -1, "depth");
	if (lua_isnumber(L, -1)) {
		settings.depth = (int) lua_tonumber(L, -1);
	} else {
		printf("RenderSettings 'depth' field is missing or not a number.\n");
		lua_pop(L, 1); // Pop the 'depth' field
		return 0;      // Error
	}
	lua_pop(L, 1); // Pop the 'depth' field

	// Read spray field
	lua_getfield(L, -1, "spray");
	if (lua_isnumber(L, -1)) {
		settings.spray = (int) lua_tonumber(L, -1);
	} else {
		printf("RenderSettings 'spray' field is missing or not a number.\n");
		lua_pop(L, 1); // Pop the 'spray' field
		return 0;      // Error
	}
	lua_pop(L, 1); // Pop the 'spray' field

	// Read depth field
	lua_getfield(L, -1, "multisampling");
	if (lua_isnumber(L, -1)) {
		settings.multisampling = (int) lua_tonumber(L, -1);
	} else {
		printf("RenderSettings 'depth' field is missing or not a number.\n");
		lua_pop(L, 1); // Pop the 'depth' field
		return 0;      // Error
	}
	lua_pop(L, 1); // Pop the 'depth' field

	return 1; // Success
}

// Function to read a Lua table representing a light
int tp::Scene::readLight(lua_State* L, tp::PointLight* light) {
	light->pos = getVec3(L, "pos");

	lua_getfield(L, -1, "intensity"); // Get the "intensity" field from the light table
	if (!lua_isnumber(L, -1)) {
		printf("Light is missing the 'intensity' field or it's not a number.\n");
		lua_pop(L, 1); // Pop the 'intensity' field
		return 0;      // Error
	}
	light->intensity = lua_tonumber(L, -1);
	lua_pop(L, 1); // Pop the 'intensity' field

	return 1; // Success
}

bool tp::Scene::loadLuaFormat(const std::string& scenePath) {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	namespace fs = std::filesystem;

	fs::path fullPath(scenePath);

	// Extract the filename
	std::string fileName = fullPath.filename().string();

	// Remove the filename from the path
	fs::path directoryPath = fullPath.remove_filename();

	if (luaL_dofile(L, scenePath.c_str()) != 0) {
		lua_close(L);
		printf("Cant open scene script.\n");
		return false;
	}

	lua_getglobal(L, "Meshes");

	if (lua_isstring(L, -1)) {
		std::string meshesPath = lua_tostring(L, -1);

		directoryPath /= meshesPath;

		if (!loadOBJFormat(directoryPath.string())) {
			printf("No 'meshes' loaded - check ur .obj path and validate content of .obj .\n");
			return false;
		}

	} else {
		printf("No 'meshes' path given.\n");
		return false;
	}

	// --- camera
	{
		lua_getglobal(L, "Camera");
		if (!lua_istable(L, -1)) {
			printf("Camera is not a table.\n");
			lua_close(L);
			return false;
		}

		Vec3F camPos = getVec3(L, "pos");
		Vec3F camTarget = getVec3(L, "target");
		Vec3F camUp = getVec3(L, "up");
		Vec2F camSize = getVec2(L, "size");

		mRenderSettings.size = camSize;

		mCamera.lookAtPoint(camTarget, camPos, camUp);
		mCamera.setFOV(3.14 / 4);
		mCamera.setFar(100);
		mCamera.setRatio((tp::halnf) camSize.y / (tp::halnf) camSize.x);

		lua_pop(L, 1);
	}

	// ---------- LIGHTS
	{
		lua_getglobal(L, "Lights");
		if (!lua_istable(L, -1)) {
			printf("Lights is not a table.\n");
			lua_close(L);
			return false; // Error
		}

		// Read and process each light in the "Lights" table
		int numLights = lua_rawlen(L, -1); // Get the number of lights in the table
		for (int i = 1; i <= numLights; i++) {
			lua_rawgeti(L, -1, i); // Get the i-th element (light) from the table
			if (lua_istable(L, -1)) {
				tp::PointLight light;
				if (!readLight(L, &light)) {
					printf("Cant read lights data\n");
					lua_close(L);
					return false; // Error
				}
				mLights.append(light);
			}
			lua_pop(L, 1); // Pop the i-th light table
		}
	}

	// ----------- settings --------------
	if (!readRenderSettings(L, mRenderSettings)) {
		printf("Cant Read Render Settings");
		lua_close(L);
		return false; // Error
	}

	lua_close(L);

	return true;
}
