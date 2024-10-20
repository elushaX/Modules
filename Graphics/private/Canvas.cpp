#include "Window.hpp"

// -------- OpenGL -------- //
#define GLEW_STATIC
#include <GL/glew.h>

#include "Graphics.hpp"

// -------- Canvas -------- //
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

namespace tp {
	class Canvas::Context {
	public:
		NVGcontext* vg {};
		Window* window {};
	};
}

using namespace tp;

Canvas::Canvas(Window* window) {
	mContext = new Context();
	mContext->window = window;

	mContext->vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

	if (nvgCreateFont(mContext->vg, "default", "Font.ttf") == -1) {
		ASSERT(!"Cant create NVG font")
	}
}

Canvas::~Canvas() {
	nvgDeleteGL3(mContext->vg);
	delete mContext;
}

void Canvas::rect(RectF rec, const RGBA& col, halnf round) {
	rec.pos += mOrigin;

	nvgBeginPath(mContext->vg);

	if (round == 0) {
		nvgRect(mContext->vg, rec.x, rec.y, rec.z, rec.w);
	} else {
		nvgRoundedRect(mContext->vg, rec.x, rec.y, rec.z, rec.w, round);
	}

	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgFill(mContext->vg);
}

void Canvas::debugCross(RectF rec, const RGBA& col) {
	nvgBeginPath(mContext->vg);

	nvgMoveTo(mContext->vg, rec.p1().x, rec.p1().y);
	nvgLineTo(mContext->vg, rec.p3().x, rec.p3().y);

	nvgMoveTo(mContext->vg, rec.p2().x, rec.p2().y);
	nvgLineTo(mContext->vg, rec.p4().x, rec.p4().y);

	nvgStrokeWidth(mContext->vg, 2);
	nvgStrokeColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgStroke(mContext->vg);
	// nvgFill(mContext->vg);
}

void Canvas::frame(RectF rec, const RGBA& col, halnf round) {
	nvgBeginPath(mContext->vg);

	nvgMoveTo(mContext->vg, rec.p1().x, rec.p1().y);
	nvgLineTo(mContext->vg, rec.p2().x, rec.p2().y);
	nvgLineTo(mContext->vg, rec.p3().x, rec.p3().y);
	nvgLineTo(mContext->vg, rec.p4().x, rec.p4().y);
	nvgLineTo(mContext->vg, rec.p1().x, rec.p1().y);

	nvgStrokeWidth(mContext->vg, 2);
	nvgStrokeColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgStroke(mContext->vg);
	// nvgFill(mContext->vg);
}

void Canvas::circle(Vec2F pos, halnf size, const RGBA& col) {
	pos += mOrigin;

	nvgBeginPath(mContext->vg);
	nvgCircle(mContext->vg, pos.x, pos.y, size);

	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgFill(mContext->vg);
}

void Canvas::setOrigin(const Vec2F& origin) {
	mOrigin = origin;
}

void Canvas::pushClamp(const RectF& rec) {
	RectF intersection = rec;
	if (mScissors.size()) {
		mScissors.last().calcIntersection(rec, intersection);
	}
	nvgScissor(mContext->vg, intersection.x, intersection.y, intersection.z, intersection.w);
	mScissors.append(intersection);
}

void Canvas::popClamp() {
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

const RectF& Canvas::getClampedArea() const {
	return mScissors.last();
}

void Canvas::text(
	const char* string, const RectF& aRec, halnf size, Align align, halnf marging, const RGBA& col
) {

	RectF rec = { aRec.x + marging, aRec.y + marging, aRec.z - marging * 2, aRec.w - marging * 2 };
	rec.pos += mOrigin;

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

void Canvas::drawImage(RectF rec, ImageHandle* image, halnf angle, halnf alpha, halnf rounding) {
	rec.pos += mOrigin;

	auto imgPaint = nvgImagePattern(mContext->vg, rec.x, rec.y, rec.z, rec.w, angle, image->id, alpha);
	nvgBeginPath(mContext->vg);
	nvgRoundedRect(mContext->vg, rec.x, rec.y, rec.z, rec.w, rounding);
	nvgFillPaint(mContext->vg, imgPaint);
	nvgFill(mContext->vg);
}

 Canvas::ImageHandle Canvas::createImageFromTextId(ualni id, Vec2F size) {
#ifdef ENV_OS_ANDROID
	 return { (ualni) nvglCreateImageFromHandleGLES3(mContext->vg, id, size.x, size.y, 0) };
#else
	 //return { (ualni) nvglCreateImageFromHandleGL3(mContext->vg, id, size.x, size.y, 0) };
	 return { (ualni) nvglCreateImageFromHandleGL3(mContext->vg, id, size.x, size.y, 0) };
#endif
}

void Canvas::updateTextureID(ImageHandle handle, ualni id) {
	auto nvgl = (GLNVGcontext*) nvgInternalParams(mContext->vg)->userPtr;
	auto glhandle = nvglImageHandleGL3(mContext->vg, handle.id);

	auto tex = glnvg__findTexture(nvgl, handle.id);
	tex->tex = id;
}

void Canvas::deleteImageHandle(ImageHandle image) {
	if (image.id) {
		nvgDeleteImage(mContext->vg, image.id);
	}
}

void Canvas::drawBegin() {
	const auto size = mContext->window->getSize();
	nvgBeginFrame(mContext->vg, size.x, size.y, 1.0);
	glViewport(0, 0, size.x, size.y);
}

void Canvas::drawEnd() {
	const auto size = mContext->window->getSize();
	glViewport(0, 0, size.x, size.y);
	nvgEndFrame(mContext->vg);
}


void Canvas::colorWheel(const RectF& rec, const ColorWheel& colorWheel) {
	auto NVG = mContext->vg;
	const auto sizeInner = 5.f;
	const auto sizeOuter = 7.f;

	float const x = rec.x + mOrigin.x;
	float const y = rec.y + mOrigin.y;
	float const w = rec.z;
	float const h = rec.w;

	const HSV& hsv = colorWheel.color;
	float const hue = hsv.h / (NVG_PI * 2);

	int i;
	float r0, r1, ax, ay, bx, by, cx, cy, aeps, r;
	NVGpaint paint;

	nvgSave(mContext->vg);

	cx = x + w * 0.5f;
	cy = y + h * 0.5f;
	r1 = (w < h ? w : h) * 0.5f - colorWheel.margin;
	r0 = r1 - colorWheel.thickness;

	aeps = 0.5f / r1;	// half a pixel arc length in radians (2pi cancels out).

	for (i = 0; i < 6; i++) {
		float a0 = (float)i / 6.0f * NVG_PI * 2.0f - aeps;
		float a1 = (float)(i + 1.0f) / 6.0f * NVG_PI * 2.0f + aeps;
		nvgBeginPath(NVG);
		nvgArc(NVG, cx, cy, r0, a0, a1, NVG_CW);
		nvgArc(NVG, cx, cy, r1, a1, a0, NVG_CCW);
		nvgClosePath(NVG);
		ax = cx + cosf(a0) * (r0 + r1) * 0.5f;
		ay = cy + sinf(a0) * (r0 + r1) * 0.5f;
		bx = cx + cosf(a1) * (r0 + r1) * 0.5f;
		by = cy + sinf(a1) * (r0 + r1) * 0.5f;
		paint = nvgLinearGradient(NVG, ax, ay, bx, by, nvgHSLA(a0 / (NVG_PI * 2), 1.0f, 0.55f, 255), nvgHSLA(a1 / (NVG_PI * 2), 1.0f, 0.55f, 255));
		nvgFillPaint(NVG, paint);
		nvgFill(NVG);
	}

	nvgBeginPath(NVG);
	nvgCircle(NVG, cx, cy, r0 - 0.5f);
	nvgCircle(NVG, cx, cy, r1 + 0.5f);
	nvgStrokeColor(NVG, nvgRGBA(0, 0, 0, 64));
	nvgStrokeWidth(NVG, 1.0f);
	nvgStroke(NVG);

	// Selector
	nvgSave(NVG);
	nvgTranslate(NVG, cx, cy);
	nvgRotate(NVG, hue * NVG_PI * 2);

	// Marker on
	nvgStrokeWidth(NVG, 2.0f);
	nvgBeginPath(NVG);
	nvgRect(NVG, r0 - 1, -3, r1 - r0 + 2, 6);
	nvgStrokeColor(NVG, nvgRGBA(255, 255, 255, 192));
	nvgStroke(NVG);

	paint = nvgBoxGradient(NVG, r0 - 3, -5, r1 - r0 + 6, 10, 2, 4, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(NVG);
	nvgRect(NVG, r0 - 2 - 10, -4 - 10, r1 - r0 + 4 + 20, 8 + 20);
	nvgRect(NVG, r0 - 2, -4, r1 - r0 + 4, 8);
	nvgPathWinding(NVG, NVG_HOLE);
	nvgFillPaint(NVG, paint);
	nvgFill(NVG);

	// Center triangle
	r = r0 - 6;
	ax = cosf(120.0f / 180.0f * NVG_PI) * r;
	ay = sinf(120.0f / 180.0f * NVG_PI) * r;
	bx = cosf(-120.0f / 180.0f * NVG_PI) * r;
	by = sinf(-120.0f / 180.0f * NVG_PI) * r;
	nvgBeginPath(NVG);
	nvgMoveTo(NVG, r, 0);
	nvgLineTo(NVG, ax, ay);
	nvgLineTo(NVG, bx, by);
	nvgClosePath(NVG);
	paint = nvgLinearGradient(NVG, r, 0, ax, ay, nvgHSLA(hue, 1.0f, 0.5f, 255), nvgRGBA(255, 255, 255, 255));
	nvgFillPaint(NVG, paint);
	nvgFill(NVG);
	paint = nvgLinearGradient(NVG, (r + ax) * 0.5f, (0 + ay) * 0.5f, bx, by, nvgRGBA(0, 0, 0, 0), nvgRGBA(0, 0, 0, 255));
	nvgFillPaint(NVG, paint);
	nvgFill(NVG);
	nvgStrokeColor(NVG, nvgRGBA(0, 0, 0, 64));
	nvgStroke(NVG);

	// Select circle on triangle
	float yt = hsv.v * hsv.s;
	float xt = hsv.v - 0.5 * yt;
	ay = sinf(120.0f / 180.0f * NVG_PI) * r * (-1.0f + xt * 2.0f);
	ax = cosf(120.0f / 180.0f * NVG_PI) * r * (1.0f - yt * 3.f);
	nvgStrokeWidth(NVG, 2.0f);
	nvgBeginPath(NVG);
	nvgCircle(NVG, ax, ay, sizeInner);
	nvgStrokeColor(NVG, nvgRGBA(255, 255, 255, 192));
	nvgStroke(NVG);

	paint = nvgRadialGradient(NVG, ax, ay, 7, 9, nvgRGBA(0, 0, 0, 64), nvgRGBA(0, 0, 0, 0));
	nvgBeginPath(NVG);
	nvgRect(NVG, ax - 20, ay - 20, 40, 40);
	nvgCircle(NVG, ax, ay, sizeOuter);
	nvgPathWinding(NVG, NVG_HOLE);
	nvgFillPaint(NVG, paint);
	nvgFill(NVG);

	nvgRestore(NVG);

	nvgRestore(NVG);
}

void Canvas::ColorWheel::fromPoint(const tp::RectF& area, const tp::Vec2F& crs) {
	auto wheel_rec = area;
	wheel_rec.pos += margin;
	wheel_rec.size -= margin * 2;

	auto center = wheel_rec.pos + wheel_rec.size / 2.f;
	auto edge = min(wheel_rec.size.x, wheel_rec.size.y);
	wheel_rec.pos = center - edge / 2.f;
	wheel_rec.size = edge;

	HSV hsv = color;

	auto pos = crs - center;

	auto r = (crs - center).length();
	if (r < edge / 2.f) {
		if (r > edge / 2.f - thickness) {
			auto angle = halnf(atan2(pos.y, pos.x));
			angle = halnf(angle > 0 ? angle : PI * 2 + angle);
			angle = clamp(angle, 0.f, halnf(PI * 2));
			hsv.h = angle;
		}
		else {
			pos.y *= -1;
			pos /= (edge / 2.f) - thickness - 10;

			auto angle = PI * 2 - hsv.h - PI / 2;
			if (angle < 0) {
				angle += PI * 2;
			}

			auto sin = halnf(::sin(-angle));
			auto cos = halnf(::cos(-angle));
			auto sv_pos = Vec2F{ pos.x * cos - pos.y * sin, pos.x * sin + pos.y * cos };

			auto sv_angle = halnf(atan2(sv_pos.x, sv_pos.y));
			sv_angle = halnf(sv_angle > 0 ? sv_angle : PI * 2 + sv_angle);
			sv_angle = clamp(sv_angle, 0.f, halnf(PI * 2));

			auto p1 = Vec2F(0, 1);
			auto p2 = Vec2F(::cos(PI / 6), -::sin(PI / 6));
			auto p3 = Vec2F{ -p2.x, p2.y };

			Vec2F intersection = sv_pos;
			if (sv_angle < PI / 3 * 2) {
				if (intersectLines2D({ 0, 0 }, sv_pos, p1, p2, &intersection)) {
					sv_pos = intersection;
				}
			}
			else if (sv_angle < PI / 3 * 4) {
				if (intersectLines2D({ 0, 0 }, sv_pos, p3, p2, &intersection)) {
					sv_pos = intersection;
				}
			}
			else {
				if (intersectLines2D({ 0, 0 }, sv_pos, p1, p3, &intersection)) {
					sv_pos = intersection;
				}
			}

			sv_pos.y = halnf((sv_pos.y + 1 / 2.f) * 2 / 3.f * 1.0);
			sv_pos.x = halnf(sv_pos.x * 2 / sqrt(3) * 1.0);
			sv_pos.x = (sv_pos.x + 1.f) / 2;

			sv_pos.x = clamp(sv_pos.x, 0.f, 1.f);
			sv_pos.y = clamp(sv_pos.y, 0.f, 1.f);

			auto v = sv_pos.y * 0.5f + sv_pos.x;
			auto s = sv_pos.y / v;

			hsv.v = clamp(v, 0.001f, 0.999f);
			hsv.s = clamp(s, 0.001f, 0.999f);
		}
	}

	color = hsv;
}