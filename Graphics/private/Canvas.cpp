#include "Window.hpp"
#include "Profiler.hpp"
#include "WindowContext.hpp"

#include "Graphics.hpp"
#include "RenderTargetRegistry.hpp"
#include "CanvasFonts.hpp"

#include <nanovg.h>
#include <nanovg_vk_api.h>

#include <vector>

namespace tp {
	class Canvas::Context {
	public:
		NVGcontext* vg{};
		NVGcontext* chrome{};
		Window* window{};
		int regularFont = -1;
		int boldFont = -1;
		int monospaceFont = -1;
	};
}

using namespace tp;

namespace {
	const NVGcolor kWindowBaseColor = nvgRGBAf(0.118f, 0.122f, 0.133f, 1.0f);
	const NVGcolor kWindowOutlineColor = nvgRGBAf(1.0f, 1.0f, 1.0f, 0.07f);
	const NVGcolor kFocusedShadowColor = nvgRGBAf(0, 0, 0, 0.42f);
	const NVGcolor kUnfocusedShadowColor = nvgRGBAf(0, 0, 0, 0.24f);
	constexpr halnf kShadowFeatherFraction = 0.9f;
	constexpr halnf kShadowOffsetY = 3;
	constexpr halnf kItalicSkew = -0.21f;

	bool isBold(Canvas::FontFace face) { return face == Canvas::FontFace::Bold || face == Canvas::FontFace::BoldItalic; }

	bool isItalic(Canvas::FontFace face) {
		return face == Canvas::FontFace::Italic || face == Canvas::FontFace::BoldItalic;
	}

	std::vector<int> loadFallbackFonts(NVGcontext* vg) {
		std::vector<int> loaded;
		for (const FontFile& font : kFallbackFonts) {
			const int id = nvgCreateFont(vg, font.name, font.path);
			if (id != -1) loaded.push_back(id);
		}
		return loaded;
	}

	int loadTextFont(NVGcontext* vg, const FontFile& font, const std::vector<int>& fallbacks, int missing) {
		const int id = nvgCreateFont(vg, font.name, font.path);
		if (id == -1) return missing;
		for (const int fallback : fallbacks)
			nvgAddFallbackFontId(vg, id, fallback);
		return id;
	}
}

int Canvas::fontIdFor(FontFace face) const {
	if (face == FontFace::Monospace) return mContext->monospaceFont;
	return isBold(face) ? mContext->boldFont : mContext->regularFont;
}

Canvas::Canvas(Window* window) {
	mContext = new Context();
	mContext->window = window;

	VulkanContext* vulkan = window->getContext()->vulkan;

	VKNVGCreateInfo createInfo{};
	createInfo.gpu = vulkan->physicalDevice();
	createInfo.device = vulkan->device();
	createInfo.renderpass = vulkan->renderPass();
	createInfo.cmdBuffer = vulkan->commandBufferArray();
	createInfo.queueFamilyIndex = vulkan->graphicsQueueFamily();
	createInfo.currentFrame = vulkan->currentFrameSlot();
	createInfo.swapchainImageCount = vulkan->framesInFlight();

	mContext->vg = nvgCreateVkApi(createInfo, NVG_ANTIALIAS | NVG_STENCIL_STROKES, vulkan->graphicsQueue());
	mContext->chrome = nvgCreateVkApi(createInfo, NVG_ANTIALIAS, vulkan->graphicsQueue());

	const std::vector<int> fallbacks = loadFallbackFonts(mContext->vg);
	mContext->regularFont = loadTextFont(mContext->vg, kRegularFont, fallbacks, -1);
	if (mContext->regularFont == -1) {
		ASSERT(!"Cant create NVG font")
	}
	mContext->boldFont = loadTextFont(mContext->vg, kBoldFont, fallbacks, mContext->regularFont);
	mContext->monospaceFont = loadTextFont(mContext->vg, kMonospaceFont, fallbacks, mContext->regularFont);
}

Canvas::~Canvas() {
	mContext->window->getContext()->vulkan->waitIdle();
	nvgDeleteVkApi(mContext->chrome);
	nvgDeleteVkApi(mContext->vg);
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

void Canvas::gradientRect(RectF rec, const RGBA& from, const RGBA& to, bool horizontal, halnf round) {
	rec.pos += mOrigin;
	auto vg = mContext->vg;
	const halnf endX = horizontal ? rec.x + rec.z : rec.x;
	const halnf endY = horizontal ? rec.y : rec.y + rec.w;
	const NVGpaint paint =
		nvgLinearGradient(vg, rec.x, rec.y, endX, endY, { from.r, from.g, from.b, from.a }, { to.r, to.g, to.b, to.a });
	nvgBeginPath(vg);
	if (round == 0) nvgRect(vg, rec.x, rec.y, rec.z, rec.w);
	else nvgRoundedRect(vg, rec.x, rec.y, rec.z, rec.w, round);
	nvgFillPaint(vg, paint);
	nvgFill(vg);
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

void Canvas::outline(RectF rec, const RGBA& col, halnf round, halnf width) {
	rec.pos += mOrigin;

	const halnf inset = width / 2;
	nvgBeginPath(mContext->vg);
	nvgRoundedRect(
		mContext->vg, rec.x + inset, rec.y + inset, rec.z - width, rec.w - width, round > inset ? round - inset : 0
	);
	nvgStrokeWidth(mContext->vg, width);
	nvgStrokeColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgStroke(mContext->vg);
}

void Canvas::circleOutline(Vec2F pos, halnf size, const RGBA& col, halnf width) {
	pos += mOrigin;

	nvgBeginPath(mContext->vg);
	nvgCircle(mContext->vg, pos.x, pos.y, size - width / 2);
	nvgStrokeWidth(mContext->vg, width);
	nvgStrokeColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgStroke(mContext->vg);
}

void Canvas::polyline(const Vec2F* points, int count, const RGBA& col, halnf width) {
	if (count < 2) return;

	nvgBeginPath(mContext->vg);
	nvgMoveTo(mContext->vg, points[0].x + mOrigin.x, points[0].y + mOrigin.y);
	for (int index = 1; index < count; index++) {
		nvgLineTo(mContext->vg, points[index].x + mOrigin.x, points[index].y + mOrigin.y);
	}
	nvgLineCap(mContext->vg, NVG_ROUND);
	nvgLineJoin(mContext->vg, NVG_ROUND);
	nvgStrokeWidth(mContext->vg, width);
	nvgStrokeColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgStroke(mContext->vg);
}

void Canvas::polygon(const Vec2F* points, int count, const RGBA& col) {
	if (count < 3) return;

	nvgBeginPath(mContext->vg);
	nvgMoveTo(mContext->vg, points[0].x + mOrigin.x, points[0].y + mOrigin.y);
	for (int index = 1; index < count; index++) {
		nvgLineTo(mContext->vg, points[index].x + mOrigin.x, points[index].y + mOrigin.y);
	}
	nvgClosePath(mContext->vg);
	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgFill(mContext->vg);
}

void Canvas::line(Vec2F start, Vec2F end, const RGBA& col, halnf thickness) {
	start += mOrigin;
	end += mOrigin;

	nvgBeginPath(mContext->vg);
	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgMoveTo(mContext->vg, start.x, start.y);
	nvgLineTo(mContext->vg, end.x, end.y);
	nvgStrokeWidth(mContext->vg, thickness);
	nvgStrokeColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgStroke(mContext->vg);
}

void Canvas::circle(Vec2F pos, halnf size, const RGBA& col) {
	pos += mOrigin;

	nvgBeginPath(mContext->vg);
	nvgCircle(mContext->vg, pos.x, pos.y, size);

	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgFill(mContext->vg);
}

void Canvas::setOrigin(const Vec2F& origin) { mOrigin = origin; }

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

const RectF& Canvas::getClampedArea() const { return mScissors.last(); }

bool Canvas::isOutsideClamp(const RectF& rec) const {
	if (!mScissors.size()) return false;

	const RectF& clip = mScissors.last();
	return rec.x >= clip.x + clip.z || clip.x >= rec.x + rec.z || rec.y >= clip.y + clip.w || clip.y >= rec.y + rec.w;
}

void Canvas::text(
	const char* string, const RectF& aRec, halnf size, Align align, halnf marging, const RGBA& col, FontFace face
) {

	RectF rec = { aRec.x + marging, aRec.y + marging, aRec.z - marging * 2, aRec.w - marging * 2 };
	rec.pos += mOrigin;
	if (isOutsideClamp(rec)) return;

	pushClamp(rec);

	nvgFontSize(mContext->vg, size);
	nvgFontFaceId(mContext->vg, fontIdFor(face));
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

	if (isItalic(face)) {
		nvgSave(mContext->vg);
		nvgTranslate(mContext->vg, centerX, centerY);
		nvgSkewX(mContext->vg, kItalicSkew);
		nvgText(mContext->vg, 0, 0, string, nullptr);
		nvgRestore(mContext->vg);
	} else {
		nvgText(mContext->vg, centerX, centerY, string, nullptr);
	}

	popClamp();
}

void Canvas::rotatedText(const char* string, Vec2F center, halnf size, halnf angle, const RGBA& col) {
	center += mOrigin;

	nvgSave(mContext->vg);
	nvgTranslate(mContext->vg, center.x, center.y);
	nvgRotate(mContext->vg, angle);
	nvgFontSize(mContext->vg, size);
	nvgFontFaceId(mContext->vg, mContext->regularFont);
	nvgTextAlign(mContext->vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgFillColor(mContext->vg, { col.r, col.g, col.b, col.a });
	nvgText(mContext->vg, 0, 0, string, nullptr);
	nvgRestore(mContext->vg);
}

halnf Canvas::textWidth(const char* string, halnf size, FontFace face) {
	nvgFontSize(mContext->vg, size);
	nvgFontFaceId(mContext->vg, fontIdFor(face));
	return nvgTextBounds(mContext->vg, 0, 0, string, nullptr, nullptr);
}

void Canvas::prefixWidths(const char* string, size_t length, halnf size, FontFace face, halnf* widths) {
	nvgFontSize(mContext->vg, size);
	nvgFontFaceId(mContext->vg, fontIdFor(face));
	nvgTextAlign(mContext->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

	std::vector<NVGglyphPosition> glyphs(length);
	const int count = nvgTextGlyphPositions(mContext->vg, 0, 0, string, string + length, glyphs.data(), (int) length);

	size_t byte = 0;
	for (int index = 0; index < count; index++) {
		const size_t glyphStart = (size_t) (glyphs[(size_t) index].str - string);
		const halnf pen = glyphs[(size_t) index].x;
		for (; byte <= glyphStart && byte < length; byte++)
			widths[byte] = pen;
	}
	const halnf total = nvgTextBounds(mContext->vg, 0, 0, string, string + length, nullptr);
	for (; byte < length; byte++)
		widths[byte] = total;
	widths[length] = total;
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
	RenderTargetEntry entry;
	if (!RenderTargetRegistry::resolve((uint32_t) id, entry)) {
		return { 0 };
	}
	int image = nvgCreateImageFromVkImageApi(mContext->vg, entry.view, entry.sampler, entry.width, entry.height);
	return { (ualni) image };
}

void Canvas::updateTextureID(ImageHandle handle, ualni id) {
	RenderTargetEntry entry;
	if (!RenderTargetRegistry::resolve((uint32_t) id, entry)) return;
	nvgUpdateVkImageApi(mContext->vg, (int) handle.id, entry.view, entry.sampler, entry.width, entry.height);
}

Canvas::ImageHandle Canvas::createImageFromFile(const char* path) {
	int image = nvgCreateImage(mContext->vg, path, 0);
	return { (ualni) image };
}

Vec2F Canvas::imageSize(ImageHandle image) {
	if (!image.id) return { 0, 0 };

	int width = 0;
	int height = 0;
	nvgImageSize(mContext->vg, (int) image.id, &width, &height);
	return { (halnf) width, (halnf) height };
}

Canvas::ImageHandle Canvas::createImageFromMemoryRGBA(int width, int height, const unsigned char* rgba) {
	int image = nvgCreateImageRGBA(mContext->vg, width, height, 0, rgba);
	return { (ualni) image };
}

void Canvas::updateImageRGBA(ImageHandle image, const unsigned char* rgba) {
	if (image.id) nvgUpdateImage(mContext->vg, (int) image.id, rgba);
}

void Canvas::deleteImageHandle(ImageHandle image) {
	if (image.id) {
		nvgDeleteImage(mContext->vg, image.id);
	}
}

void Canvas::drawBegin() {
	const WindowFrame& frame = mContext->window->frame();
	const Vec2F surfaceSize = frame.surfaceSize();
	const Vec2F framebufferSize = mContext->window->getFramebufferSize();
	const halnf devicePixelRatio = surfaceSize.x > 0 ? framebufferSize.x / surfaceSize.x : 1.0f;

	nvgBeginFrame(mContext->vg, surfaceSize.x, surfaceSize.y, devicePixelRatio);
	nvgTranslate(mContext->vg, frame.margin(), frame.margin());
	fillWindowBase(frame);
}

void Canvas::drawEnd() {
	TP_PROFILE_SCOPE("frame.ui_flush");
	nvgEndFrame(mContext->vg);
}

void Canvas::drawWindowChrome() {
	const WindowFrame& frame = mContext->window->frame();
	if (!frame.hasShadow()) return;

	const Vec2F surfaceSize = frame.surfaceSize();
	const Vec2F framebufferSize = mContext->window->getFramebufferSize();
	const halnf devicePixelRatio = surfaceSize.x > 0 ? framebufferSize.x / surfaceSize.x : 1.0f;
	NVGcontext* chrome = mContext->chrome;

	nvgBeginFrame(chrome, surfaceSize.x, surfaceSize.y, devicePixelRatio);
	cutRoundedCorners(chrome, frame);
	paintOutline(chrome, frame);
	paintShadow(chrome, frame, mContext->window->isFocused());
	nvgEndFrame(chrome);
}

void Canvas::fillWindowBase(const WindowFrame& frame) {
	const Vec2F size = frame.contentSize();

	nvgBeginPath(mContext->vg);
	nvgRect(mContext->vg, 0, 0, size.x, size.y);
	nvgFillColor(mContext->vg, kWindowBaseColor);
	nvgFill(mContext->vg);
}

void Canvas::cutRoundedCorners(NVGcontext* vg, const WindowFrame& frame) {
	const RectF content = frame.contentRect();

	nvgSave(vg);
	nvgGlobalCompositeOperation(vg, NVG_DESTINATION_OUT);
	nvgBeginPath(vg);
	nvgRect(vg, content.x, content.y, content.z, content.w);
	nvgRoundedRect(vg, content.x, content.y, content.z, content.w, frame.cornerRadius());
	nvgPathWinding(vg, NVG_HOLE);
	nvgFillColor(vg, nvgRGBAf(0, 0, 0, 1));
	nvgFill(vg);
	nvgRestore(vg);
}

void Canvas::paintOutline(NVGcontext* vg, const WindowFrame& frame) {
	const RectF content = frame.contentRect();
	const halnf inset = 0.5f;

	nvgBeginPath(vg);
	nvgRoundedRect(
		vg, content.x + inset, content.y + inset, content.z - inset * 2, content.w - inset * 2, frame.cornerRadius() - inset
	);
	nvgStrokeWidth(vg, 1);
	nvgStrokeColor(vg, kWindowOutlineColor);
	nvgStroke(vg);
}

void Canvas::paintShadow(NVGcontext* vg, const WindowFrame& frame, bool focused) {
	const RectF content = frame.contentRect();
	const Vec2F surface = frame.surfaceSize();
	const halnf feather = frame.margin() * kShadowFeatherFraction;
	const NVGcolor shadow = focused ? kFocusedShadowColor : kUnfocusedShadowColor;

	const NVGpaint paint = nvgBoxGradient(
		vg,
		content.x,
		content.y + kShadowOffsetY,
		content.z,
		content.w,
		frame.cornerRadius(),
		feather,
		shadow,
		nvgRGBAf(0, 0, 0, 0)
	);

	nvgSave(vg);
	nvgGlobalCompositeOperation(vg, NVG_DESTINATION_OVER);
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, surface.x, surface.y);
	nvgFillPaint(vg, paint);
	nvgFill(vg);
	nvgRestore(vg);
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

	aeps = 0.5f / r1; // half a pixel arc length in radians (2pi cancels out).

	for (i = 0; i < 6; i++) {
		float a0 = (float) i / 6.0f * NVG_PI * 2.0f - aeps;
		float a1 = (float) (i + 1.0f) / 6.0f * NVG_PI * 2.0f + aeps;
		nvgBeginPath(NVG);
		nvgArc(NVG, cx, cy, r0, a0, a1, NVG_CW);
		nvgArc(NVG, cx, cy, r1, a1, a0, NVG_CCW);
		nvgClosePath(NVG);
		ax = cx + cosf(a0) * (r0 + r1) * 0.5f;
		ay = cy + sinf(a0) * (r0 + r1) * 0.5f;
		bx = cx + cosf(a1) * (r0 + r1) * 0.5f;
		by = cy + sinf(a1) * (r0 + r1) * 0.5f;
		paint = nvgLinearGradient(
			NVG, ax, ay, bx, by, nvgHSLA(a0 / (NVG_PI * 2), 1.0f, 0.55f, 255), nvgHSLA(a1 / (NVG_PI * 2), 1.0f, 0.55f, 255)
		);
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
		} else {
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
			} else if (sv_angle < PI / 3 * 4) {
				if (intersectLines2D({ 0, 0 }, sv_pos, p3, p2, &intersection)) {
					sv_pos = intersection;
				}
			} else {
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