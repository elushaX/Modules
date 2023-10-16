
#pragma once

#include "Buffer2D.hpp"
#include "Camera.hpp"
#include "Color.hpp"
#include "Module.hpp"
#include "Topology.hpp"
#include "Vec.hpp"

namespace tp {

  extern ModuleManifest gModuleRayTracer;

  class Scene {
  public:
    typedef Topology Object;

  public:
    Scene() = default;

  public:
    Buffer<Object> mObjects;
    Camera mCamera;
  };

  class RayTracer {
  public:
    typedef Buffer2D<RGBA> RenderBuffer;

    struct RenderSettings {
      alni samplesiPerPixel = 1;
      alni rayBounces = 1;
      Vec2I size;
    };

  public:
    RayTracer() = default;
    void render(const Scene& scene, RenderBuffer& buff);

  private:
    struct RayCastData {
      const Scene::Object* obj = nullptr;
      TrigCache* trig = nullptr;
      Vec3F hitPos = {0, 0, 0};
      bool hit = false;
    };

    void castRay(const Ray& ray, RayCastData& out, alnf far);

  private:
    RenderSettings mSettings;
    const Scene* mScene = nullptr;
    RenderBuffer* mBuff = nullptr;
  };
}
