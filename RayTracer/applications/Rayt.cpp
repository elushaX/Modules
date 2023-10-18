
// #include "NewPlacement.hpp"

#include "Rayt.hpp"

#include "CommandLine.hpp"
#include "Timing.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace tp;

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
    printf("Image saved successfully.\n");
  } else {
    printf("Error saving the image.\n");
  }
}

static void* printStatus(void* arg) {
  auto percentage = (const halnf*) arg;
  auto old = *percentage;
  while (*percentage < 0.99f) {
    sleep(500);
    if (old != *percentage && (*percentage - old) > 0.01) {
      printf("Progress - %f\n", (*percentage) * 100);
      old = *percentage;
    }
  }
  pthread_exit(NULL);
}

void renderCommand(const String& scenePath) {
  Scene scene;

  RayTracer::RenderSettings settings;

  loadScene(scene, scenePath, settings);

  RayTracer::RenderBuffer output;
  output.reserve(RayTracer::RenderBuffer::Index2D(settings.size.x, settings.size.y));

  RayTracer rayt;

  pthread_t my_thread;
  int ret;

  printf("\nStarting Render:\n\n");

  ret = pthread_create(&my_thread, NULL, &printStatus, &rayt.mProgress.percentage);
  if (ret != 0) {
    printf("Error: pthread_create() failed\n");
    exit(EXIT_FAILURE);
  }

  auto start = get_time();

  rayt.render(scene, output, settings);

  auto end = get_time();

  pthread_join(my_thread, NULL);

  printf("\nRender finished with average render time per sample - %i (ms)\n", end - start);

  writeImage(output);
}

int main(int argc, const char** argv) {
  tp::ModuleManifest* deps[] = {&gModuleRayTracer, nullptr};
  tp::ModuleManifest module("Rayt", nullptr, nullptr, deps);

  if (module.initialize()) {
    renderCommand("scene.lua");

    module.deinitialize();
  }

  return 0;
}
