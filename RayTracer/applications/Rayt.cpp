
// #include "NewPlacement.hpp"

#include "Buffer.hpp"
#include "RayTracer.hpp"
#include "Strings.hpp"
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
    // Go through each vertex and print its number,
    //  position, normal, and texture coordinate
    for (int j = 0; j < curMesh.Vertices.size(); j++) {
      // file << "V" << j << ": "
      //     << "P(" << curMesh.Vertices[j].Position.X << ", " << curMesh.Vertices[j].Position.Y << ", " << curMesh.Vertices[j].Position.Z << ") "
      //     << "N(" << curMesh.Vertices[j].Normal.X << ", " << curMesh.Vertices[j].Normal.Y << ", " << curMesh.Vertices[j].Normal.Z << ") "
      //     << "TC(" << curMesh.Vertices[j].TextureCoordinate.X << ", " << curMesh.Vertices[j].TextureCoordinate.Y << ")\n";
    }

    // Print Indices
    // Go through every 3rd index and print the
    //	triangle that these indices represent
    for (int j = 0; j < curMesh.Indices.size(); j += 3) {
      // file << "T" << j / 3 << ": " << curMesh.Indices[j] << ", " << curMesh.Indices[j + 1] << ", " << curMesh.Indices[j + 2] << "\n";
    }
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

  RayTracer::RenderBuffer output;
  output.reserve(size);

  RayTracer rayt;

  rayt.render(scene, output);

  writeImage(output);
}

int main() {
  tp::ModuleManifest* deps[] = {&gModuleRayTracer, nullptr};
  tp::ModuleManifest module("Rayt", nullptr, nullptr, deps);

  if (module.initialize()) {
    renderCommand("scene.obj", {1000, 1000});

    module.deinitialize();
  }

  return 0;
}
