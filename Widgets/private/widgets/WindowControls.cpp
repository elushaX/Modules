#include "WindowControls.hpp"

#include "BoxLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kGlyphHalfSize = 4.5f;
	constexpr halnf kGlyphThickness = 1.2f;
	constexpr halnf kRestoreOffset = 2;
	constexpr halnf kControlsGap = 2;

	RGBA withAlpha(const RGBA& color, halnf alpha) { return { color.r, color.g, color.b, alpha }; }

	const char* debugNameOf(WindowButton::Kind kind) {
		switch (kind) {
			case WindowButton::Kind::Minimize: return "minimize button";
			case WindowButton::Kind::Maximize: return "maximize button";
			case WindowButton::Kind::Close: return "close button";
		}
		return "window button";
	}
}

WindowButton::WindowButton(Kind kind) :
	mKind(kind) {
	setStyle(Style::Flat);
	setDebugName(debugNameOf(kind));
}

Extent WindowButton::measureContent(Axis axis, halnf) const {
	const Theme& style = theme();
	return Extent::rigid(axis == Axis::Horizontal ? style.metrics.windowButtonWidth : style.metrics.iconButtonSize);
}

RGBA WindowButton::backgroundTarget() const {
	const Palette& palette = theme().palette;

	if (mKind == Kind::Close) {
		if (isPressed()) return palette.windowClosePressed;
		if (isHovered()) return palette.windowCloseHovered;
		return withAlpha(palette.windowCloseHovered, 0);
	}

	if (isPressed()) return palette.controlPressed;
	if (isHovered()) return palette.controlHovered;
	return withAlpha(palette.controlHovered, 0);
}

void WindowButton::paint(Painter& painter) {
	const Theme& style = theme();
	const bool closeHighlighted = mKind == Kind::Close && (isHovered() || isPressed());
	const RGBA glyph = closeHighlighted ? style.palette.onWindowClose
										 : isHovered()    ? style.palette.text
																			: style.palette.mutedText;

	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);
	paintGlyph(painter, localRect().center(), glyph);
}

void WindowButton::paintGlyph(Painter& painter, Vec2F center, const RGBA& color) const {
	const Vec2F half = { kGlyphHalfSize, kGlyphHalfSize };

	switch (mKind) {
		case Kind::Minimize:
			painter.line(center + Vec2F(-kGlyphHalfSize, 0.0f), center + Vec2F(kGlyphHalfSize, 0.0f), color, kGlyphThickness);
			return;

		case Kind::Maximize:
			{
				const WindowHost* host = ui() ? ui()->windowHost() : nullptr;
				const bool maximized = host && host->isWindowMaximized();
				const RectF box = makeRect(center - half, half * 2.0f);

				if (!maximized) {
					painter.strokeRect(box, color, 2, kGlyphThickness);
					return;
				}

				const Vec2F shift = { kRestoreOffset, -kRestoreOffset };
				painter.strokeRect(translated(insetRect(box, 1), shift), color, 2, kGlyphThickness);
				painter.fillRect(insetRect(box, 1), background(), 2);
				painter.strokeRect(insetRect(box, 1), color, 2, kGlyphThickness);
				return;
			}

		case Kind::Close:
			painter.line(center - half, center + half, color, kGlyphThickness);
			painter.line(
				center + Vec2F(-kGlyphHalfSize, kGlyphHalfSize),
				center + Vec2F(kGlyphHalfSize, -kGlyphHalfSize),
				color,
				kGlyphThickness
			);
			return;
	}
}

void WindowButton::onActivated() {
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (!host) return;

	switch (mKind) {
		case Kind::Minimize: host->minimizeWindow(); break;
		case Kind::Maximize: host->toggleMaximizeWindow(); break;
		case Kind::Close: host->closeWindow(); break;
	}
}

WindowControls::WindowControls() {
	setDebugName("window controls");
	setSizing(AxisSizing::fit(), AxisSizing::fit());
	emplaceLayout<RowLayout>(kControlsGap).setCrossAlign(CrossAlign::Center);

	addChild(mMinimize);
	addChild(mMaximize);
	addChild(mClose);
}

void WindowControls::onAttached() { followWindowHost(); }

void WindowControls::onWindowHostChanged() { followWindowHost(); }

void WindowControls::followWindowHost() {
	const WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	setVisible(host && host->platformInfo().hasWindowControls);
}
