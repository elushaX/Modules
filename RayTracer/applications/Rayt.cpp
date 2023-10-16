
// #include "NewPlacement.hpp"

#include "Buffer.hpp"
#include "CommandLine.hpp"
#include "RayTracer.hpp"
#include "Strings.hpp"
#include "Timing.hpp"
#include "Vec.hpp"

#include "OBJ_Loader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace tp;

void loadScene(Scene& scene, const String& scenePath) {
  objl::Loader Loader;

  if (!Loader.LoadFile(scenePath.read())) {
    std::cout << "Failed to Load File. May have failed to find it or it was not an .obj file.\n";
    return;
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
}

void writeImage(const RayTracer::RenderBuffer& output) {
  // Save the data to a PNG file
  struct urgb {
    uint1 r, g, b, a;
  };

  Buffer2D<urgb> converted;
  converted.reserve(output.size());

  for (RayTracer::RenderBuffer::Index i = 0; i < output.size().x; i++) {
    for (RayTracer::RenderBuffer::Index j = 0; j < output.size().y; j++) {
      converted.get({i, j}).r = uint1(output.get({i, j}).r * 255);
      converted.get({i, j}).g = uint1(output.get({i, j}).g * 255);
      converted.get({i, j}).b = uint1(output.get({i, j}).b * 255);
      converted.get({i, j}).a = uint1(output.get({i, j}).a * 255);
    }
  }

  if (stbi_write_png("output.png", converted.size().x, converted.size().y, 4, converted.getBuff(), converted.size().x * 4) != 0) {
    // Image saved successfully
    std::cout << "Image saved successfully." << std::endl;
  } else {
    std::cerr << "Error saving the image." << std::endl;
  }
}

void renderCommand(const String& scenePath, RayTracer::RenderBuffer::Index2D size) {
  Scene scene;

  loadScene(scene, scenePath);

  scene.mCamera.lookAtPoint({0, 0, 0}, {-3, -3, 3}, {1, 1, 1});
  scene.mCamera.setFOV(3.14 / 4);
  scene.mCamera.setFar(100);
  scene.mCamera.setRatio((halnf) size.y / (halnf) size.x);

  RayTracer::RenderBuffer output;
  output.reserve(size);

  RayTracer rayt;

  auto start = get_time();

  rayt.render(scene, output);

  auto end = get_time();

  printf("\n Render finished with average render time per sample - %i (ms)\n", end - start);

  writeImage(output);
}

int main(int argc, const char** argv) {
  tp::ModuleManifest* deps[] = {&gModuleRayTracer, nullptr};
  tp::ModuleManifest module("Rayt", nullptr, nullptr, deps);

  if (module.initialize()) {
    renderCommand("scene.obj", {600, 600});

    module.deinitialize();
  }

  return 0;
}
