#include "TaskStateChip.hpp"

#include "Painter.hpp"
#include "TaskAppearance.hpp"
#include "Theme.hpp"

#include <cmath>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kChipDot = 7;
	constexpr halnf kChipPadding = 6;
	constexpr halnf kChipGap = 5;
	constexpr halnf kChipHeight = 18;
	constexpr halnf kPulsePeriodMs = 260;

	RGBA withAlpha(RGBA color, halnf alpha) {
		color.a *= alpha;
		return color;
	}
}

TaskStateChip::TaskStateChip() {
	setDebugName("task state chip");
	setSizing(AxisSizing::fit(), AxisSizing::fit());
}

void TaskStateChip::show(std::string text, const RGBA& color, bool pulsing) {
	const bool resized = text != mText;
	mText = std::move(text);
	mColor = color;
	mPulsing = pulsing;
	if (mPulsing) startTicking();
	if (resized) requestLayout();
	requestPaint();
}

void TaskStateChip::setDotShown(bool shown) {
	if (shown == mDotShown) return;
	mDotShown = shown;
	requestLayout();
}

halnf TaskStateChip::textStart() const { return mDotShown ? kChipPadding + kChipDot + kChipGap : kChipPadding; }

Extent TaskStateChip::measureContent(Axis axis, halnf) const {
	if (axis == Axis::Vertical) return Extent::rigid(kChipHeight);
	return Extent::rigid(textStart() + kChipPadding + textWidth(mText, theme().metrics.smallFontSize));
}

void TaskStateChip::onTick(halnf deltaMs) {
	mClockMs += deltaMs;
	requestPaint();
	if (!mPulsing) stopTicking();
}

void TaskStateChip::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF rect = localRect();
	const halnf pulse = mPulsing ? 0.55f + 0.45f * std::sin(mClockMs / kPulsePeriodMs) : 1.0f;

	painter.fillRect(rect, withAlpha(mColor, mDotShown ? 0.18f : 0.32f), rect.size.y / 2);
	if (mDotShown) {
		const Vec2F dot = { kChipPadding + kChipDot / 2, rect.size.y / 2 };
		painter.fillCircle(dot, kChipDot / 2, withAlpha(mColor, pulse));
	}
	const RectF textBox = makeRect(textStart(), 0, rect.size.x - textStart() - kChipPadding, rect.size.y);
	painter.text(mText, textBox, style.metrics.smallFontSize, TextAlign::Start, style.palette.text);
}

void TaskStateChip::showSession(SessionState state) {
	setVisible(state != SessionState::None);
	show(std::string(sessionStateLabel(state)), sessionStateColor(state), isSessionActive(state));
}
