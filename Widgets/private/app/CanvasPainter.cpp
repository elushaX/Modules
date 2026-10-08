#include "CanvasPainter.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	Canvas::Align canvasAlign(TextAlign align) {
		switch (align) {
			case TextAlign::Start: return Canvas::LC;
			case TextAlign::Center: return Canvas::CC;
			case TextAlign::End: return Canvas::RC;
		}
		return Canvas::LC;
	}

	Canvas::FontFace canvasFace(FontFace face) {
		switch (face) {
			case FontFace::Regular: return Canvas::FontFace::Regular;
			case FontFace::Bold: return Canvas::FontFace::Bold;
			case FontFace::Italic: return Canvas::FontFace::Italic;
			case FontFace::BoldItalic: return Canvas::FontFace::BoldItalic;
			case FontFace::Monospace: return Canvas::FontFace::Monospace;
		}
		return Canvas::FontFace::Regular;
	}
}

CanvasPainter::CanvasPainter(Canvas& canvas) :
	mCanvas(canvas),
	mOrigins{ { 0, 0 } } {
	mCanvas.setOrigin(origin());
}

void CanvasPainter::pushTranslation(Vec2F offset) {
	mOrigins.push_back(origin() + offset);
	mCanvas.setOrigin(origin());
}

void CanvasPainter::popTranslation() {
	if (mOrigins.size() > 1) mOrigins.pop_back();
	mCanvas.setOrigin(origin());
}

void CanvasPainter::pushClip(const RectF& rect) { mCanvas.pushClamp(translated(rect, origin())); }

void CanvasPainter::popClip() { mCanvas.popClamp(); }

void CanvasPainter::drawFilledRect(const RectF& rect, const RGBA& color, halnf cornerRadius) {
	mCanvas.rect(rect, color, cornerRadius);
}

void CanvasPainter::drawStrokedRect(const RectF& rect, const RGBA& color, halnf cornerRadius, halnf width) {
	mCanvas.outline(rect, color, cornerRadius, width);
}

void CanvasPainter::drawStrokedCircle(Vec2F center, halnf radius, const RGBA& color, halnf width) {
	mCanvas.circleOutline(center, radius, color, width);
}

void CanvasPainter::polyline(const std::vector<Vec2F>& points, const RGBA& color, halnf thickness) {
	mCanvas.polyline(points.data(), (int) points.size(), color, thickness);
}

void CanvasPainter::fillPolygon(const std::vector<Vec2F>& points, const RGBA& color) {
	mCanvas.polygon(points.data(), (int) points.size(), color);
}

void CanvasPainter::fillCircle(Vec2F center, halnf radius, const RGBA& color) { mCanvas.circle(center, radius, color); }

void CanvasPainter::line(Vec2F from, Vec2F to, const RGBA& color, halnf thickness) {
	mCanvas.line(from, to, color, thickness);
}

void CanvasPainter::styledText(
	const std::string& text, const RectF& box, const TextStyle& style, TextAlign align, const RGBA& color
) {
	mCanvas.text(text.c_str(), box, style.fontSize, canvasAlign(align), 0, color, canvasFace(style.face));
}

void CanvasPainter::verticalText(
	const std::string& text, const RectF& box, halnf fontSize, TextDirection direction, const RGBA& color
) {
	const halnf quarterTurn = 1.5707964f;
	const halnf angle = direction == TextDirection::BottomToTop ? -quarterTurn : quarterTurn;
	mCanvas.rotatedText(text.c_str(), box.center(), fontSize, angle, color);
}

void CanvasPainter::drawImage(const RectF& rect, ImageId image, halnf cornerRadius) {
	Canvas::ImageHandle handle = { image.value };
	mCanvas.drawImage(rect, &handle, 0, 1, cornerRadius);
}

void CanvasPainter::colorWheel(const RectF& rect, const HSV& selected, halnf ringThickness) {
	Canvas::ColorWheel wheel;
	wheel.color = selected;
	wheel.thickness = ringThickness;
	wheel.margin = 0;
	mCanvas.colorWheel(rect, wheel);
}

void CanvasPainter::fillGradientRect(
	const RectF& rect, const RGBA& from, const RGBA& to, bool horizontal, halnf cornerRadius
) {
	mCanvas.gradientRect(rect, from, to, horizontal, cornerRadius);
}

halnf CanvasPainter::styledTextWidth(const std::string& text, const TextStyle& style) const {
	return mCanvas.textWidth(text.c_str(), style.fontSize, canvasFace(style.face));
}

std::vector<halnf> CanvasPainter::prefixWidths(const std::string& text, const TextStyle& style) const {
	std::vector<halnf> widths(text.size() + 1, 0);
	mCanvas.prefixWidths(text.c_str(), text.size(), style.fontSize, canvasFace(style.face), widths.data());
	return widths;
}

bool CanvasPainter::isClippedOut(const RectF& rect) const { return mCanvas.isOutsideClamp(translated(rect, origin())); }
