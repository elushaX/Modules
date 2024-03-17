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

void Graphics::Canvas::drawColorwheel(const RectF& rec, const RGB& col) {

	float const x = rec.x;
	float const y = rec.y;
	float const w = rec.z;
	float const h = rec.w;

	HSV hsv = tp::HSV(col);
	float const hue = hsv.h / (NVG_PI * 2);

	int i;
	float r0, r1, ax, ay, bx, by, cx, cy, aeps, r;
	NVGpaint paint;

	nvgSave(mContext->vg);

	cx = x + w * 0.5f;
	cy = y + h * 0.5f;
	r1 = (w < h ? w : h) * 0.5f - 5.0f;
	r0 = r1 - 13.0f;
	aeps = 0.5f / r1; // half a pixel arc length in radians (2pi cancels out).

	for (i = 0; i < 6; i++) {
		float a0 = (float) i / 6.0f * NVG_PI * 2.0f - aeps;
		float a1 = (float) (i + 1.0f) / 6.0f * NVG_PI * 2.0f + aeps;
		nvgBeginPath(mContext->vg);
		nvgArc(mContext->vg, cx, cy, r0, a0, a1, NVG_CW);
		nvgArc(mContext->vg, cx, cy, r1, a1, a0, NVG_CCW);
		nvgClosePath(mContext->vg);
		ax = cx + cosf(a0) * (r0 + r1) * 0.5f;
		ay = cy + sinf(a0) * (r0 + r1) * 0.5f;
		bx = cx + cosf(a1) * (r0 + r1) * 0.5f;
		by = cy + sinf(a1) * (r0 + r1) * 0.5f;
		paint = nvgLinearGradient(
			mContext->vg,
			ax,
			ay,
			bx,
			by,
			nvgHSLA(a0 / (NVG_PI * 2), 1.0f, 0.55f, 255),
			nvgHSLA(a1 / (NVG_PI * 2), 1.0f, 0.55f, 255)
		);
		nvgFillPaint(mContext->vg, paint);
		nvgFill(mContext->vg);
	}

	nvgBeginPath(mContext->vg);
	nvgCircle(mContext->vg, cx, cy, r0 - 0.5f);
	nvgCircle(mContext->vg, cx, cy, r1 + 0.5f);
	nvgStrokeColor(mContext->vg, nvgRGBA(0, 0, 0, 64));
	nvgStrokeWidth(mContext->vg, 1.0f);
	nvgStroke(mContext->vg);

	// Selector
	nvgSave(mContext->vg);
	nvgTranslate(mContext->vg, cx, cy);
	nvgRotate(mContext->vg, hue * NVG_PI * 2);

	// Marker on
	nvgStrokeWidth(mContext->vg, 2.0f);
	nvgBeginPath(mContext->vg);
	nvgRect(mContext->vg, r0 - 1, -3, r1 - r0 + 2, 6);
	nvgStrokeColor(mContext->vg, nvgRGBA(255, 255, 255, 192));
	nvgStroke(mContext->vg);

	paint = nvgBoxGradient(mContext->vg, r0 - 3, -5, r1 - r0 + 6, 10, 2, 4, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(mContext->vg);
	nvgRect(mContext->vg, r0 - 2 - 10, -4 - 10, r1 - r0 + 4 + 20, 8 + 20);
	nvgRect(mContext->vg, r0 - 2, -4, r1 - r0 + 4, 8);
	nvgPathWinding(mContext->vg, NVG_HOLE);
	nvgFillPaint(mContext->vg, paint);
	nvgFill(mContext->vg);

	// Center triangle
	r = r0 - 6;
	ax = cosf(120.0f / 180.0f * NVG_PI) * r;
	ay = sinf(120.0f / 180.0f * NVG_PI) * r;
	bx = cosf(-120.0f / 180.0f * NVG_PI) * r;
	by = sinf(-120.0f / 180.0f * NVG_PI) * r;
	nvgBeginPath(mContext->vg);
	nvgMoveTo(mContext->vg, r, 0);
	nvgLineTo(mContext->vg, ax, ay);
	nvgLineTo(mContext->vg, bx, by);
	nvgClosePath(mContext->vg);
	paint = nvgLinearGradient(mContext->vg, r, 0, ax, ay, nvgHSLA(hue, 1.0f, 0.5f, 255), nvgRGBA(255, 255, 255, 255));
	nvgFillPaint(mContext->vg, paint);
	nvgFill(mContext->vg);
	paint = nvgLinearGradient(mContext->vg, (r + ax) * 0.5f, (0 + ay) * 0.5f, bx, by, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 255));
	nvgFillPaint(mContext->vg, paint);
	nvgFill(mContext->vg);
	nvgStrokeColor(mContext->vg, nvgRGBA(0, 0, 0, 64));
	nvgStroke(mContext->vg);

	// Select circle on triangle
	float yt = hsv.v * hsv.s;
	float xt = hsv.v - 0.5 * yt;
	ay = sinf(120.0f / 180.0f * NVG_PI) * r * (-1.0f + xt * 2.0f);
	ax = cosf(120.0f / 180.0f * NVG_PI) * r * (1.0f - yt * 3.f);
	nvgStrokeWidth(mContext->vg, 2.0f);
	nvgBeginPath(mContext->vg);
	nvgCircle(mContext->vg, ax, ay, 5);
	nvgStrokeColor(mContext->vg, nvgRGBA(255, 255, 255, 192));
	nvgStroke(mContext->vg);

	paint = nvgRadialGradient(mContext->vg, ax, ay, 7, 9, nvgRGBA(0, 0, 0, 64), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(mContext->vg);
	nvgRect(mContext->vg, ax - 20, ay - 20, 40, 40);
	nvgCircle(mContext->vg, ax, ay, 7);
	nvgPathWinding(mContext->vg, NVG_HOLE);
	nvgFillPaint(mContext->vg, paint);
	nvgFill(mContext->vg);

	nvgRestore(mContext->vg);

	nvgRestore(mContext->vg);
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
