
#include "Editor.hpp"

#include "OpenImageDenoise/oidn.hpp"
#include <iostream>

using namespace tp;

void Editor::denoise() {
	auto& out = mPathTracerBuffers;

	// Initialize OIDN device
	oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
	device.commit();

	// Define buffer size
	const int channels = 4; // ARGB
	const auto size = out.color.size();
	// Create the denoising filter
	oidn::FilterRef filter = device.newFilter("RT"); // Use the 'RT' filter for path tracing

	// Set the input and output buffer (same buffer for in-place denoising)
	filter.setImage("color", out.color.getBuff(), oidn::Format::Float3, size.x, size.y, 0, sizeof(float) * channels); // ARGB
	filter.setImage("output", out.color.getBuff(), oidn::Format::Float3, size.x, size.y, 0, sizeof(float) * channels); // ARGB

	// Set additional parameters if needed
	filter.set("hdr", false); // Assuming the input image is not HDR

	// Commit the filter
	filter.commit();

	// Execute the filter
	filter.execute();

	// Check for errors
	const char* errorMessage;
	if (device.getError(errorMessage) != oidn::Error::None) {
		std::cerr << "Error: " << errorMessage << std::endl;
	}
}
