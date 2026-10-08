#include "Theme.hpp"

#include <initializer_list>

using namespace tp;
using namespace tp::ui;

namespace {
	const RGBA kWhite = { 1, 1, 1, 1 };
	const RGBA kBlack = { 0, 0, 0, 1 };
}

RGBA Palette::mix(const RGBA& from, const RGBA& to, halnf amount) {
	return { from.r + (to.r - from.r) * amount,
					 from.g + (to.g - from.g) * amount,
					 from.b + (to.b - from.b) * amount,
					 from.a + (to.a - from.a) * amount };
}

void Palette::setAccent(const RGBA& color) {
	accent = { color.r, color.g, color.b, 1.0f };
	accentHovered = mix(accent, kWhite, 0.12f);
	accentPressed = mix(accent, kBlack, 0.12f);
	focusRing = { accent.r, accent.g, accent.b, 0.55f };
	selection = mix(surface, accent, 0.45f);
}

namespace {
	Insets scaledInsets(const Insets& insets, halnf factor) {
		return { insets.left * factor, insets.top * factor, insets.right * factor, insets.bottom * factor };
	}
}

Metrics Metrics::scaled(halnf factor) const {
	Metrics result = *this;
	for (halnf* length : { &result.fontSize,
												 &result.smallFontSize,
												 &result.lineHeight,
												 &result.cornerRadius,
												 &result.popupCornerRadius,
												 &result.surfaceCornerRadius,
												 &result.indicatorCornerRadius,
												 &result.surfaceGap,
												 &result.controlHeight,
												 &result.iconButtonSize,
												 &result.windowButtonWidth,
												 &result.indicatorSize,
												 &result.spacing,
												 &result.sliderTrackThickness,
												 &result.sliderHandleSize,
												 &result.sliderTickLength,
												 &result.sliderTickThickness,
												 &result.sliderTickSpacing,
												 &result.scrollBarThickness,
												 &result.scrollTrackThickness,
												 &result.scrollTrackEdgeGap,
												 &result.splitterThickness,
												 &result.menuItemHeight,
												 &result.popupGap })
		*length *= factor;
	result.controlPadding = scaledInsets(controlPadding, factor);
	result.menuPadding = scaledInsets(menuPadding, factor);
	result.tooltipPadding = scaledInsets(tooltipPadding, factor);
	return result;
}

Theme Theme::scaled(halnf factor) const {
	Theme theme = *this;
	theme.metrics = metrics.scaled(factor);
	return theme;
}

Theme Theme::withAccent(const RGBA& accent) const {
	Theme theme = *this;
	theme.palette.setAccent(accent);
	return theme;
}

const Theme& Theme::standard() {
	static const Theme standardTheme;
	return standardTheme;
}
