
#include "NewPlacement.hpp"

#include "CommandLine.hpp"
#include "Module.hpp"
#include "RayTracer.hpp"

using namespace tp;

ModuleManifest* sDependencies[] = {&gModuleMath, &gModuleCommandLine, &gModuleConnection, nullptr};

ModuleManifest tp::gModuleRayTracer = ModuleManifest("RayTracer", nullptr, nullptr, sDependencies);

void RayTracer::render(const Scene& scene, RayTracer::RenderBuffer& buff) {
  for (RayTracer::RenderBuffer::Index i = 0; i < buff.size().x; i++) {
    for (RayTracer::RenderBuffer::Index j = 0; j < buff.size().y; j++) {
      buff.set({i, j}, RGBA(1.f, 1.f, 1.f, 1.f));
    }
  }
}
