
#include "MathCommon.hpp"
#include "NewPlacement.hpp"

#include "CommandLine.hpp"
#include "Module.hpp"
#include "Ray.hpp"
#include "RayTracer.hpp"
#include "TypeInfo.hpp"

#include <cstdio>

using namespace tp;

ModuleManifest* sDependencies[] = {&gModuleMath, &gModuleCommandLine, &gModuleConnection, nullptr};

ModuleManifest tp::gModuleRayTracer = ModuleManifest("RayTracer", nullptr, nullptr, sDependencies);

void RayTracer::castRay(const Ray& ray, RayCastData& out, alnf far) {
  out.hit = false;

  far *= far;

  for (auto obj : mScene->mObjects) {
    for (auto trig : obj->getTrigs()) {
      if (trig->castRay(ray)) {
        // printf("Hit\n");

        auto dist = (trig->getHitPos() - ray.pos).length2();

        if (far > dist) {
          out.trig = &trig.data();
          out.hitPos = trig->getHitPos();
          out.obj = &obj.data();
          out.hit = true;

          far = dist;
        }
      }
    }
  }
}

void RayTracer::render(const Scene& scene, RayTracer::RenderBuffer& buff) {
  mScene = &scene;
  mBuff = &buff;

  auto pos = mScene->mCamera.getPos();
  auto fov = mScene->mCamera.getFOV();
  auto height = sqrt(mScene->mCamera.getRatio());
  auto width = 1.f / height;
  auto forward = mScene->mCamera.getForward();
  auto up = mScene->mCamera.getUp();
  auto right = forward.cross(up);
  auto planeCenter = pos + (forward * halnf(width / (2.f * tan(fov / 2.f))));
  auto planeCenterOffset = (up * (halnf) height / 2.f) - (right * (halnf) width / 2.f);

  auto planeLeftTop = planeCenter + planeCenterOffset;

  RayCastData castData;

  Ray ray = {
      {0, 0, 0},
      pos
  };

  Vec3F iterPoint = {0, 0, 0};
  Vec3F deltaX = right * halnf(width / (alnf) buff.size().x);
  Vec3F deltaY = up * halnf(-height / (alnf) buff.size().y);

  for (RayTracer::RenderBuffer::Index i = 0; i < buff.size().x; i++) {
    for (RayTracer::RenderBuffer::Index j = 0; j < buff.size().y; j++) {
      iterPoint = planeLeftTop + ((deltaX * (halnf) i) + (deltaY * (halnf) j));

      ray.dir = (iterPoint - pos).unitV();

      castRay(ray, castData, mScene->mCamera.getFar());

      if (castData.hit) {
        auto normal = castData.trig->getNormal();
        normal.normalize();

        RGBA col = {normal.x * 0.5f + 0.5f, normal.y * 0.5f + 0.5f, normal.z * 0.5f + 0.5f, 1.f};
        auto depth = 1 - clamp((halnf) (castData.hitPos - ray.pos).length(), 0.f, mScene->mCamera.getFar()) / mScene->mCamera.getFar();

        // buff.set({i, j}, {depth, depth, depth, 1.f});
        buff.set({i, j}, col);

      } else {
        buff.set({i, j}, RGBA(0.f, 0.f, 0.f, 0.f));
      }
    }
  }
}
