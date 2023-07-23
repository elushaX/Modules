#include "Window.hpp"

// -------- OpenGL -------- //
#include <GL/glew.h>

#include "WindowContext.hpp"

// -------- Canvas -------- //
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

namespace tp {
	class Graphics::Canvas::Context {
	public:
		NVGcontext* vg;
	};
}

using namespace tp;

Graphics::Canvas::Canvas() {
	mContext = new Context();
}

Graphics::Canvas::~Canvas() {
	delete mContext;
}

void Graphics::Canvas::init() {
	mContext->vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
}

void Graphics::Canvas::deinit() {
	// Cleanup
	nvgDeleteGL3(mContext->vg);
}

void Graphics::Canvas::proc() {
	auto width = 600;
	auto height = 600;

	// Start NanoVG rendering
	nvgBeginFrame(mContext->vg, width, height, 1.0);

	// Draw a rectangle
	nvgBeginPath(mContext->vg);
	nvgRect(mContext->vg, 50, 50, 50, 50);
	nvgFillColor(mContext->vg, nvgRGBf(0.8f, 0.4f, 0.1f));
	nvgFill(mContext->vg);

	// End NanoVG rendering
	nvgEndFrame(mContext->vg);
}

void Graphics::Canvas::draw() {
	// End NanoVG rendering
	nvgEndFrame(mContext->vg);
}