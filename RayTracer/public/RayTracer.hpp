
#pragma once

#include "Buffer2D.hpp"
#include "Camera.hpp"
#include "Color.hpp"
#include "Module.hpp"
#include "Topology.hpp"

namespace tp {

  extern ModuleManifest gModuleRayTracer;

  class Scene {
  public:
    Scene() = default;

  public:
    Buffer<Topology> mObjects;
    Camera mCamera;
  };

  class RayTracer {
  public:
    typedef Buffer2D<RGBA> RenderBuffer;

  public:
    RayTracer() = default;
    void render(const Scene& scene, RenderBuffer& buff);
  };
}
