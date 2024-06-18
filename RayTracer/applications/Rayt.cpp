
// #include "NewPlacement.hpp"

#include "RayTracer.hpp"

#include "Timing.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tp;

void writeImage(const RayTracer::RenderBuffer& output, const char* name) {
	// Save the data to a PNG file
	struct urgb {
		uint1 r, g, b, a;
	};

	Buffer2D<urgb> converted;
	converted.reserve(output.size());

	for (Index i = 0; i < output.size().x; i++) {
		for (Index j = 0; j < output.size().y; j++) {
			converted.get({ i, j }).r = uint1(output.get({ i, j }).r * 255);
			converted.get({ i, j }).g = uint1(output.get({ i, j }).g * 255);
			converted.get({ i, j }).b = uint1(output.get({ i, j }).b * 255);
			converted.get({ i, j }).a = uint1(output.get({ i, j }).a * 255);
		}
	}

	if (stbi_write_png(name, converted.size().x, converted.size().y, 4, converted.getBuff(), converted.size().x * 4) != 0) {
		// Image saved successfully
		printf("Image saved successfully.\n");
	} else {
		printf("Error saving the image.\n");
	}
}

void printStatus(const halnf* percentage) {
	auto old = *percentage;
	while (*percentage < 0.99f) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (old != *percentage && (*percentage - old) > 0.01) {
			std::cout << "Progress - " << (*percentage) * 100 << std::endl;
			old = *percentage;
		}
	}
}

void renderCommand(const std::string& scenePath) {
	Scene scene;
	RenderSettings& settings = scene.mRenderSettings;

	if (!scene.load(scenePath)) {
		return;
	}

	RayTracer::OutputBuffers output;
	RayTracer rayt;

	std::thread statusThread(printStatus, &rayt.mProgress.percentage);

	std::cout << "\nStarting Render:\n\n";

	auto start = get_time();

	rayt.render(scene, output, settings);

	auto end = get_time();

	statusThread.join();

	std::cout << "\nRender finished with average render time per sample - " << (end - start) << " (ms)\n";

	writeImage(output.normals, "normal.png");
	writeImage(output.color, "color.png");
	writeImage(output.depth, "depth.png");
	writeImage(output.albedo, "albedo.png");
}

int main(int argc, const char** argv) {
	if (argc > 1) {
		renderCommand(argv[1]);
	} else {
		renderCommand("scene.lua");
	}
	return 0;
}