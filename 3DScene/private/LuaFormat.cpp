
#include "Scene.hpp"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#include <filesystem>

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
int readLight(lua_State* L, tp::PointLight* light) {
	lua_getfield(L, -1, "pos"); // Get the "pos" field from the light table
	if (!lua_istable(L, -1)) {
		printf("Light is missing the 'pos' table.\n");
		return 0; // Error
	}
	for (int i = 0; i < 3; i++) {
		lua_rawgeti(L, -1, i + 1); // Index is 1-based in Lua
		if (!lua_isnumber(L, -1)) {
			printf("Light 'pos' field is not a number at index %d.\n", i);
			lua_pop(L, 2); // Pop both the number and the 'pos' table
			return 0;      // Error
		}
		light->pos[i] = lua_tonumber(L, -1);
		lua_pop(L, 1); // Pop the number
	}
	lua_pop(L, 1); // Pop the 'pos' table

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

	// Access Camera table
	lua_getglobal(L, "Camera");
	if (!lua_istable(L, -1)) {
		printf("Camera is not a table.\n");
		lua_close(L);
		return false;
	}

	// Verify you are inside the "Camera" table
	int cameraTableIndex = lua_gettop(L); // Get the index of the "Camera" table

	// Access the "pos" field and validate it
	lua_getfield(L, cameraTableIndex, "pos");
	if (!lua_istable(L, -1) || lua_rawlen(L, -1) != 3) {
		printf("Invalid 'pos' field in Camera table.\n");
		lua_close(L);
		return false;
	}

	// Read the values from the table
	float pos[3];
	for (int i = 0; i < 3; i++) {
		lua_rawgeti(L, -1, i + 1);
		if (lua_isnumber(L, -1)) {
			pos[i] = lua_tonumber(L, -1);
		} else {
			printf("Invalid 'pos' field value at index %d.\n", i + 1);
			lua_close(L);
			return false;
		}
		lua_pop(L, 1);
	}

	// Access the "size_x" field and validate it
	lua_getfield(L, cameraTableIndex, "size_x");
	if (!lua_isnumber(L, -1)) {
		printf("Invalid or missing 'size_x' field in Camera table.\n");
		lua_close(L);
		return false;
	}
	int size_x = lua_tointeger(L, -1);
	lua_pop(L, 1); // Pop the 'size_x' value from the stack

	// Access the "size_y" field and validate it
	lua_getfield(L, cameraTableIndex, "size_y");
	if (!lua_isnumber(L, -1)) {
		printf("Invalid or missing 'size_y' field in Camera table.\n");
		lua_close(L);
		return false;
	}
	int size_y = lua_tointeger(L, -1);

	mRenderSettings.size = { (tp::halnf) size_x, (tp::halnf) size_y };

	mCamera.lookAtPoint({ 0, 0, 0 }, { pos[0], pos[1], pos[2] }, { 0, 0, 1 });
	mCamera.setFOV(3.14 / 4);
	mCamera.setFar(100);
	mCamera.setRatio((tp::halnf) size_y / (tp::halnf) size_x);

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
