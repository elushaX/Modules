
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
    for (auto trig : obj->mCache.TrigCaches) {
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
        Vec3F normal = castData.trig->getNormal();

        if (1) {
          const auto& points = castData.obj->mCache.TransformedPoints;
          const auto& normals = castData.obj->mCache.TransformedNormals;
          const auto trig = castData.trig;

          const auto& n1 = normals[trig->mP1];
          const auto& n2 = normals[trig->mP2];
          const auto& n3 = normals[trig->mP3];

          auto v0 = points[trig->mP1];
          auto v1 = points[trig->mP2];
          auto v2 = points[trig->mP3];

          // Calculate barycentric coordinates
          Vec3F barycentric;

          // Calculate the area of the triangle
          auto areaABC = (halnf) (v1 - v0).cross(v2 - v0).length();
          auto areaPBC = (halnf) (v1 - castData.hitPos).cross(v2 - castData.hitPos).length();
          auto areaPCA = (halnf) (v2 - castData.hitPos).cross(v0 - castData.hitPos).length();

          // Calculate the barycentric coordinates
          barycentric.x = areaPBC / areaABC;
          barycentric.y = areaPCA / areaABC;
          barycentric.z = 1.0f - barycentric.x - barycentric.y;

          // Interpolate the normal using barycentric coordinates
          normal = n1 * barycentric.x + n2 * barycentric.y + n3 * barycentric.z;
        }

        normal.normalize();

        RGBA col = {normal.x * 0.5f + 0.5f, normal.y * 0.5f + 0.5f, normal.z * 0.5f + 0.5f, 1.f};
        buff.set({i, j}, col);

        // auto depth = 1 - clamp((halnf) (castData.hitPos - ray.pos).length(), 0.f, mScene->mCamera.getFar()) / mScene->mCamera.getFar();
        // buff.set({i, j}, {depth, depth, depth, 1.f});

      } else {
        buff.set({i, j}, RGBA(0.f, 0.f, 0.f, 0.f));
      }
    }
  }
}
