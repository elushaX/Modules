#pragma once

#include "Mat.hpp"

namespace tp {

	// Converts an OpenGL-style clip space to Vulkan's for solid mesh rendering:
	//  - flip Y: Vulkan's clip space has +Y pointing down relative to OpenGL, so
	//    without this the offscreen render is mirrored vertically.
	//  - remap Z from the OpenGL [-w, w] range to Vulkan's [0, w], since the
	//    fragment shader reads gl_FragCoord.z directly for depth shading.
	// Note: the Y flip mirrors screen-space X handedness for the viewer, so the
	// 3D navigation (orbit/pan) compensates the horizontal mouse delta.
	inline Mat4F vulkanClipCorrection() {
		Mat4F c;
		c[0] = Vec4F(1, 0, 0, 0);
		c[1] = Vec4F(0, -1, 0, 0);
		c[2] = Vec4F(0, 0, 0.5f, 0.5f);
		c[3] = Vec4F(0, 0, 0, 1);
		return c;
	}

	inline Mat4F vulkanCameraMatrix(const Mat4F& glCameraMatrix) {
		return vulkanClipCorrection() * glCameraMatrix;
	}
}
