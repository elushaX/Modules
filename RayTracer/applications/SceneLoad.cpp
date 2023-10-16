
#include "Rayt.hpp"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

#include "OBJ_Loader.h"

bool loadMeshes(tp::Scene& scene, const tp::String& objetsPath) {
  using namespace tp;

  objl::Loader Loader;

  if (!Loader.LoadFile(objetsPath.read())) {
    std::cout << "Failed to Load File. May have failed to find it or it was not an .obj file.\n";
    return false;
  }

  for (auto& curMesh : Loader.LoadedMeshes) {
    scene.mObjects.append(Topology());
    auto object = &scene.mObjects.last();

    for (int j = 0; j < curMesh.Indices.size(); j += 3) {
      unsigned int idx1 = curMesh.Indices[j];
      unsigned int idx2 = curMesh.Indices[j + 1];
      unsigned int idx3 = curMesh.Indices[j + 2];

      Vec3F v1 = {curMesh.Vertices[idx1].Position.X, curMesh.Vertices[idx1].Position.Y, curMesh.Vertices[idx1].Position.Z};
      Vec3F v2 = {curMesh.Vertices[idx2].Position.X, curMesh.Vertices[idx2].Position.Y, curMesh.Vertices[idx2].Position.Z};
      Vec3F v3 = {curMesh.Vertices[idx3].Position.X, curMesh.Vertices[idx3].Position.Y, curMesh.Vertices[idx3].Position.Z};

      object->addTrig(v1, v2, v3);
    }
    object->updateTransformed();
  }

  return scene.mObjects.size();
}

bool getVector(lua_State* L, const char* name, tp::Vec3F& out, const char* parent) {
  lua_getglobal(L, name);

  if (lua_istable(L, -1)) {
    lua_pushstring(L, "x");
    lua_gettable(L, -2);

    if (lua_isnumber(L, -1)) {
      out.x = lua_tonumber(L, -1);
    } else {
      printf("Component of vector '%s' is not found in %s\n\n", name, parent);
      return false;
    }

    lua_pop(L, 1);

  } else {
    printf("Vector '%s' is not found in %s\n\n", name, parent);
    return false;
  }
}

void loadScene(tp::Scene& scene, const tp::String& scenePath, tp::RayTracer::RenderSettings& settings) {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);

  if (luaL_dofile(L, "script.lua") != 0) {
    lua_close(L);
    printf("Cant open scene script.\n");
    return;
  }

  lua_getglobal(L, "Meshes");

  if (lua_isstring(L, -1)) {
    tp::String meshesPath = lua_tostring(L, -1);

    if (!loadMeshes(scene, meshesPath)) {
      printf("No 'meshes' loaded - check ur .obj path and validate content of .obj .\n");
      return;
    }

  } else {
    printf("No 'meshes' path given.\n");
    return;
  }

  // --- camera

  // Access Camera table
  lua_getglobal(L, "Camera");
  if (!lua_istable(L, -1)) {
    printf("Camera is not a table.\n");
    lua_close(L);
    return;
  }

  // Verify you are inside the "Camera" table
  int cameraTableIndex = lua_gettop(L); // Get the index of the "Camera" table

  // Access the "pos" field and validate it
  lua_getfield(L, cameraTableIndex, "pos");
  if (!lua_istable(L, -1) || lua_rawlen(L, -1) != 3) {
    printf("Invalid 'pos' field in Camera table.\n");
    lua_close(L);
    return;
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
      return;
    }
    lua_pop(L, 1);
  }

  // Access the "size_x" field and validate it
  lua_getfield(L, cameraTableIndex, "size_x");
  if (!lua_isnumber(L, -1)) {
    printf("Invalid or missing 'size_x' field in Camera table.\n");
    lua_close(L);
    return;
  }
  int size_x = lua_tointeger(L, -1);
  lua_pop(L, 1); // Pop the 'size_x' value from the stack

  // Access the "size_y" field and validate it
  lua_getfield(L, cameraTableIndex, "size_y");
  if (!lua_isnumber(L, -1)) {
    printf("Invalid or missing 'size_y' field in Camera table.\n");
    lua_close(L);
    return;
  }
  int size_y = lua_tointeger(L, -1);

  settings.size = {(tp::halnf) size_x, (tp::halnf) size_y};

  scene.mCamera.lookAtPoint({0, 0, 0}, {pos[0], pos[1], pos[2]}, {0, 0, 1});
  scene.mCamera.setFOV(3.14 / 4);
  scene.mCamera.setFar(100);
  scene.mCamera.setRatio((tp::halnf) size_y / (tp::halnf) size_x);

  lua_close(L);
}
