#include "Window.hpp"

// -------- OpenGL -------- //
#define GLEW_STATIC
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

Graphics::Canvas::Canvas() { mContext = new Context(); }

Graphics::Canvas::~Canvas() { delete mContext; }

void Graphics::Canvas::init() {
	mContext->vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

	if (nvgCreateFont(mContext->vg, "default", "Font.ttf") == -1) {
		// TODO
	}
}

void Graphics::Canvas::deinit() { nvgDeleteGL3(mContext->vg); }

RectF Graphics::Canvas::getAvaliableArea() { return { (halnf) 0, (halnf) 0, mWidth, mHeight }; }

void Graphics::Canvas::rect(const RectF& rec, const RGBA& col, halnf round) {
	nvgBeginPath(mContext->vg);

	if (round == 0) {
		nvgRect(mContext->vg, rec.x, rec.y, rec.z, rec.w);
	} else {
		nvgRoundedRect(mContext->vg, rec.x, rec.y, rec.z, rec.w, round);
	}

	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgFill(mContext->vg);
}

void Graphics::Canvas::pushClamp(const RectF& rec) {
	RectF intersection = rec;
	if (mScissors.size()) {
		mScissors.last().calcIntersection(rec, intersection);
	}
	nvgScissor(mContext->vg, intersection.x, intersection.y, intersection.z, intersection.w);
	mScissors.append(rec);
}

void Graphics::Canvas::popClamp() {
	DEBUG_ASSERT(mScissors.size());
	mIsClamping = false;
	mScissors.pop();
	if (mScissors.size()) {
		const auto& rec = mScissors.last();
		nvgScissor(mContext->vg, rec.x, rec.y, rec.z, rec.w);
	} else {
		nvgResetScissor(mContext->vg);
	}
}

void Graphics::Canvas::text(
	const char* string, const RectF& aRec, halnf size, Align align, halnf marging, const RGBA& col
) {
	RectF rec = { aRec.x + marging, aRec.y + marging, aRec.z - marging * 2, aRec.w - marging * 2 };

	pushClamp(rec);

	nvgFontSize(mContext->vg, size);
	nvgFontFace(mContext->vg, "default");
	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });

	float centerX = rec.x;
	float centerY = rec.y;
	int alignNVG = 0;

	if (((int1*) &align)[1] == 0x00) { // center x
		alignNVG |= NVG_ALIGN_CENTER;
		centerX += rec.z * 0.5f;
	} else if (((int1*) &align)[1] == 0x01) { // left x
		alignNVG |= NVG_ALIGN_LEFT;
	} else if (((int1*) &align)[1] == 0x02) { // right x
		alignNVG |= NVG_ALIGN_RIGHT;
		centerX += rec.z;
	}

	if (((int1*) &align)[0] == 0x00) { // center y
		alignNVG |= NVG_ALIGN_MIDDLE;
		centerY += rec.w * 0.5f;
	} else if (((int1*) &align)[0] == 0x01) { // top y
		alignNVG |= NVG_ALIGN_TOP;
		centerY += rec.w;
	} else if (((int1*) &align)[0] == 0x02) { // bottom y
		alignNVG |= NVG_ALIGN_BOTTOM;
	}

	nvgTextAlign(mContext->vg, alignNVG);

	nvgText(mContext->vg, centerX, centerY, string, nullptr);

	popClamp();
}

void Graphics::Canvas::drawImage(const RectF& rec, ImageHandle* image, halnf angle, halnf alpha, halnf rounding) {
	auto imgPaint = nvgImagePattern(mContext->vg, rec.x, rec.y, rec.z, rec.w, angle, image->id, alpha);
	nvgBeginPath(mContext->vg);
	nvgRoundedRect(mContext->vg, rec.x, rec.y, rec.z, rec.w, rounding);
	nvgFillPaint(mContext->vg, imgPaint);
	nvgFill(mContext->vg);
}

 Graphics::Canvas::ImageHandle Graphics::Canvas::createImageFromTextId(ualni id, Vec2F size) {
#ifdef ENV_OS_ANDROID
	 return { (ualni) nvglCreateImageFromHandleGLES3(mContext->vg, id, size.x, size.y, 0) };
#else
	 return { (ualni) nvglCreateImageFromHandleGL3(mContext->vg, id, size.x, size.y, 0) };
#endif
}

void Graphics::Canvas::deleteImageHandle(ImageHandle image) {
	if (image.id) {
		nvgDeleteImage(mContext->vg, image.id);
	}
}

void Graphics::Canvas::proc() { nvgBeginFrame(mContext->vg, mWidth, mHeight, 1.0); }

void Graphics::Canvas::draw() { nvgEndFrame(mContext->vg); }
