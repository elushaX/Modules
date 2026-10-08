#include "RecordingPainter.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

void RecordingPainter::pushTranslation(Vec2F offset) { mOrigins.push_back(mOrigins.back() + offset); }

void RecordingPainter::popTranslation() {
	if (mOrigins.size() > 1) mOrigins.pop_back();
}

void RecordingPainter::pushClip(const RectF& rect) {
	const RectF windowRect = translated(rect, mOrigins.back());
	mClips.push_back(mClips.empty() ? windowRect : intersection(mClips.back(), windowRect));
}

void RecordingPainter::popClip() {
	if (!mClips.empty()) mClips.pop_back();
}

void RecordingPainter::drawFilledRect(const RectF& rect, const RGBA& color, halnf cornerRadius) {
	record(PaintCommand::Kind::FillRect, rect, color, {}, cornerRadius);
}

void RecordingPainter::drawStrokedRect(const RectF& rect, const RGBA& color, halnf cornerRadius, halnf) {
	record(PaintCommand::Kind::StrokeRect, rect, color, {}, cornerRadius);
}

void RecordingPainter::drawStrokedCircle(Vec2F center, halnf radius, const RGBA& color, halnf) {
	record(
		PaintCommand::Kind::StrokeCircle, makeRect(center - Vec2F(radius, radius), Vec2F(radius * 2, radius * 2)), color
	);
}

void RecordingPainter::fillPolygon(const std::vector<Vec2F>& points, const RGBA& color) {
	if (points.empty()) return;

	Vec2F low = points.front();
	Vec2F high = points.front();
	for (const Vec2F& point : points) {
		low = { std::min(low.x, point.x), std::min(low.y, point.y) };
		high = { std::max(high.x, point.x), std::max(high.y, point.y) };
	}
	record(PaintCommand::Kind::Polygon, makeRect(low, high - low), color);
}

void RecordingPainter::polyline(const std::vector<Vec2F>& points, const RGBA& color, halnf) {
	if (points.empty()) return;

	Vec2F low = points.front();
	Vec2F high = points.front();
	for (const Vec2F& point : points) {
		low = { std::min(low.x, point.x), std::min(low.y, point.y) };
		high = { std::max(high.x, point.x), std::max(high.y, point.y) };
	}
	record(PaintCommand::Kind::Polyline, makeRect(low, high - low), color);
	for (const Vec2F& point : points)
		mCommands.back().points.push_back(point + mOrigins.back());
}

void RecordingPainter::fillCircle(Vec2F center, halnf radius, const RGBA& color) {
	record(PaintCommand::Kind::Circle, makeRect(center - Vec2F(radius, radius), Vec2F(radius * 2, radius * 2)), color);
}

void RecordingPainter::line(Vec2F from, Vec2F to, const RGBA& color, halnf) {
	record(PaintCommand::Kind::Line, makeRect(from, to - from), color);
}

void RecordingPainter::styledText(
	const std::string& text, const RectF& box, const TextStyle& style, TextAlign, const RGBA& color
) {
	record(PaintCommand::Kind::Text, box, color, text);
	mCommands.back().face = style.face;
	mCommands.back().fontSize = style.fontSize;
}

void RecordingPainter::drawImage(const RectF& rect, ImageId, halnf) { record(PaintCommand::Kind::Image, rect, {}); }

void RecordingPainter::colorWheel(const RectF& rect, const HSV&, halnf) {
	record(PaintCommand::Kind::ColorWheel, rect, {});
}

void RecordingPainter::fillGradientRect(const RectF& rect, const RGBA& from, const RGBA& to, bool, halnf cornerRadius) {
	record(PaintCommand::Kind::Gradient, rect, from, {}, cornerRadius);
	mCommands.back().endColor = to;
}

void RecordingPainter::verticalText(
	const std::string& text, const RectF& box, halnf, TextDirection, const RGBA& color
) {
	record(PaintCommand::Kind::Text, box, color, text);
}

halnf RecordingPainter::styledTextWidth(const std::string& text, const TextStyle& style) const {
	return MonospaceTextMetrics().styledTextWidth(text, style);
}

bool RecordingPainter::isClippedOut(const RectF& rect) const {
	return !mClips.empty() && !overlaps(translated(rect, mOrigins.back()), mClips.back());
}

const PaintCommand* RecordingPainter::findText(const std::string& text) const {
	for (const PaintCommand& command : mCommands) {
		if (command.kind == PaintCommand::Kind::Text && command.text == text) return &command;
	}
	return nullptr;
}

void RecordingPainter::record(
	PaintCommand::Kind kind, const RectF& localRect, const RGBA& color, const std::string& text, halnf cornerRadius
) {
	const RectF clip = mClips.empty() ? makeRect(-1.0e9f, -1.0e9f, 2.0e9f, 2.0e9f) : mClips.back();
	mCommands.push_back({ kind, translated(localRect, mOrigins.back()), color, cornerRadius, text, clip });
}
