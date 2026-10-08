#include "Button.hpp"

#include "Theme.hpp"

using namespace tp;
using namespace tp::ui;

Button::Button(std::string text, std::function<void()> onClick) :
	mText(std::move(text)) {
	setOnActivate(std::move(onClick));
	setDebugName("button");
}

void Button::setText(std::string text) {
	if (text == mText) return;

	mText = std::move(text);
	requestLayout();
}

void Button::setFontFace(FontFace face) {
	if (face == mFontFace) return;
	mFontFace = face;
	requestLayout();
}

void Button::setIcon(ImageId icon) {
	mIcon = icon;
	requestLayout();
}

void Button::setIconPlacement(IconPlacement placement) {
	mIconPlacement = placement;
	requestLayout();
}

void Button::setStyle(Style style) {
	mStyle = style;
	refreshVisualState();
}

void Button::setSelected(bool selected) {
	if (selected == mSelected) return;

	mSelected = selected;
	refreshVisualState();
}

Extent Button::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	const halnf padding = metrics.controlPadding.total(axis);
	const halnf textWidthValue = mText.empty() ? 0 : textWidth(mText, labelStyle());
	const halnf icon = hasIcon() ? iconSize() : 0;
	const bool stacked = hasIcon() && !mText.empty() && mIconPlacement == IconPlacement::Above;

	if (axis == Axis::Horizontal) {
		if (stacked) return Extent::natural((textWidthValue > icon ? textWidthValue : icon) + padding);

		const halnf gap = hasIcon() && !mText.empty() ? metrics.spacing : 0;
		return Extent::natural(icon + gap + textWidthValue + padding);
	}

	const halnf content = stacked ? icon + metrics.lineHeight : (icon > metrics.lineHeight ? icon : metrics.lineHeight);
	const halnf height = content + padding;
	return Extent::natural(height > metrics.controlHeight ? height : metrics.controlHeight);
}

void Button::paint(Painter& painter) {
	const Theme& style = theme();
	const ContentBoxes boxes = contentBoxes();

	const halnf radius = cornerRadius();
	const bool primary = (mSelected && mStyle == Style::Normal) || mStyle == Style::Accent;

	painter.fillRect(localRect(), background(), radius);
	if (mStyle == Style::Normal && !primary) painter.strokeRect(localRect(), style.palette.controlBorder, radius);
	if (showsFocusRing()) painter.strokeRect(insetRect(localRect(), -2), style.palette.focusRing, radius + 2, 2);

	if (hasIcon()) paintIcon(painter, boxes.icon);
	if (mText.empty()) return;

	const bool leadingIcon = hasIcon() && mIconPlacement == IconPlacement::Leading;
	painter.styledText(mText, boxes.text, labelStyle(), leadingIcon ? TextAlign::Start : TextAlign::Center, labelColor());
}

RGBA Button::labelColor() const {
	const Palette& palette = theme().palette;
	if (!isEffectivelyEnabled()) return palette.disabledText;

	const bool primary = (mSelected && mStyle == Style::Normal) || mStyle == Style::Accent;
	if (primary) return palette.onAccent;
	const bool quietUnselected = (mStyle == Style::Tab || mStyle == Style::Flat) && !mSelected && !isHovered();
	return quietUnselected ? palette.mutedText : palette.text;
}

void Button::setFill(std::optional<RGBA> fill) {
	mFill = fill;
	refreshVisualState();
}

RGBA Button::backgroundTarget() const {
	const Palette& palette = theme().palette;

	if (mFill && isEffectivelyEnabled()) {
		if (isPressed()) return palette.controlPressed;
		if (isHovered()) return palette.controlHovered;
		return *mFill;
	}

	if (mStyle == Style::Tab || mStyle == Style::Flat) {
		const RGBA transparent = { palette.controlHovered.r, palette.controlHovered.g, palette.controlHovered.b, 0 };
		if (!isEffectivelyEnabled()) return transparent;
		if (isPressed()) return palette.controlPressed;
		if (mSelected) return isHovered() ? palette.controlHovered : palette.neutralSelection;
		if (isHovered()) return palette.controlHovered;
		return transparent;
	}

	if (mSelected || mStyle == Style::Accent) {
		if (!isEffectivelyEnabled()) return palette.control;
		if (isPressed()) return palette.accentPressed;
		return isHovered() ? palette.accentHovered : palette.accent;
	}

	return Control::backgroundTarget();
}

void Button::paintIcon(Painter& painter, const RectF& box) { painter.image(box, mIcon); }

Button::ContentBoxes Button::contentBoxes() const {
	const Metrics& metrics = theme().metrics;
	const RectF content = metrics.controlPadding.shrink(localRect());
	const halnf icon = iconSize();

	if (!hasIcon()) return { makeRect(0, 0, 0, 0), content };

	if (mText.empty()) {
		const Vec2F center = content.center();
		return { makeRect(center.x - icon / 2, center.y - icon / 2, icon, icon), content };
	}

	if (mIconPlacement == IconPlacement::Above) {
		const RectF iconBox = makeRect(content.pos.x + (content.size.x - icon) / 2, content.pos.y, icon, icon);
		const RectF textBox = makeRect(content.pos.x, content.pos.y + icon, content.size.x, content.size.y - icon);
		return { iconBox, textBox };
	}

	const RectF iconBox = makeRect(content.pos.x, content.pos.y + (content.size.y - icon) / 2, icon, icon);
	const halnf textLeft = content.pos.x + icon + metrics.spacing;
	return { iconBox, makeRect(textLeft, content.pos.y, content.pos.x + content.size.x - textLeft, content.size.y) };
}

halnf Button::cornerRadius() const { return mCornerRadius >= 0 ? mCornerRadius : theme().metrics.cornerRadius; }

halnf Button::iconSize() const { return mIconSize > 0 ? mIconSize : theme().metrics.lineHeight + 2; }

halnf Button::labelFontSize() const {
	const bool stacked = hasIcon() && mIconPlacement == IconPlacement::Above;
	return stacked ? theme().metrics.smallFontSize : theme().metrics.fontSize;
}

namespace {
	constexpr halnf kChevronHalfHeight = 5;
	constexpr halnf kChevronDepth = 3;
	constexpr halnf kGlyphStroke = 1.5f;

	const char* glyphDebugName(GlyphButton::Glyph glyph) {
		switch (glyph) {
			case GlyphButton::Glyph::Options: return "options button";
			case GlyphButton::Glyph::Hide: return "hide button";
			case GlyphButton::Glyph::Previous: return "previous button";
			case GlyphButton::Glyph::Next: return "next button";
			case GlyphButton::Glyph::Up: return "up button";
		}
		return "glyph button";
	}
}

GlyphButton::GlyphButton(Glyph glyph) :
	mGlyph(glyph) {
	setStyle(Style::Flat);
	setDebugName(glyphDebugName(glyph));
}

Extent GlyphButton::measureContent(Axis, halnf) const { return Extent::rigid(theme().metrics.iconButtonSize); }

void GlyphButton::paint(Painter& painter) {
	const Theme& style = theme();
	RGBA color = isHovered() && isEffectivelyEnabled() ? style.palette.text : style.palette.mutedText;
	if (!isEffectivelyEnabled()) color = style.palette.disabledText;
	const Vec2F center = localRect().center();

	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);

	switch (mGlyph) {
		case Glyph::Hide: painter.line(center - Vec2F(5.0f, 0.0f), center + Vec2F(5.0f, 0.0f), color, kGlyphStroke); return;
		case Glyph::Previous: paintChevron(painter, -1, color); return;
		case Glyph::Next: paintChevron(painter, 1, color); return;
		case Glyph::Up: paintUpChevron(painter, color); return;
		case Glyph::Options:
			for (halnf offset : { -5.0f, 0.0f, 5.0f })
				painter.fillCircle(center + Vec2F(0.0f, offset), 1.5f, color);
			return;
	}
}

void GlyphButton::paintChevron(Painter& painter, halnf direction, const RGBA& color) const {
	const Vec2F center = localRect().center();
	const Vec2F tip = center + Vec2F(direction * kChevronDepth, 0.0f);
	const Vec2F back = center - Vec2F(direction * kChevronDepth, 0.0f);
	painter.polyline(
		{ back - Vec2F(0.0f, kChevronHalfHeight), tip, back + Vec2F(0.0f, kChevronHalfHeight) }, color, kGlyphStroke
	);
}

void GlyphButton::paintUpChevron(Painter& painter, const RGBA& color) const {
	const Vec2F center = localRect().center();
	const Vec2F tip = center - Vec2F(0.0f, kChevronDepth);
	const Vec2F back = center + Vec2F(0.0f, kChevronDepth);
	painter.polyline(
		{ back - Vec2F(kChevronHalfHeight, 0.0f), tip, back + Vec2F(kChevronHalfHeight, 0.0f) }, color, kGlyphStroke
	);
}

RGBA GlyphButton::backgroundTarget() const {
	const Palette& palette = theme().palette;
	const RGBA hovered = palette.controlHovered;
	if (isPressed()) return palette.controlPressed;
	if (isHovered()) return hovered;
	return { hovered.r, hovered.g, hovered.b, 0 };
}
