#include "ThemeEditor.hpp"

#include "Ui.hpp"
#include "WindowHost.hpp"

#include <cmath>
#include <cstdio>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kSwatchSize = 18;
	constexpr halnf kColorColumnWidth = 300;
	constexpr halnf kEditorColumnWidth = 300;

	std::string number(halnf value) {
		char text[32];
		std::snprintf(text, sizeof(text), "%.3ff", value);
		return text;
	}

	std::string twoDecimals(halnf value) {
		char text[32];
		std::snprintf(text, sizeof(text), "%.2f", value);
		return text;
	}

	const RGBA kAccentPresets[] = { accents::blue, accents::purple, accents::green, accents::orange, accents::pink };
}

const std::vector<ThemeColorField>& tp::ui::themeColorFields() {
	static const std::vector<ThemeColorField> fields = {
		{ "window", &Palette::window },
		{ "surface", &Palette::surface },
		{ "raisedSurface", &Palette::raisedSurface },
		{ "border", &Palette::border },
		{ "focusRing", &Palette::focusRing },
		{ "control", &Palette::control },
		{ "controlHovered", &Palette::controlHovered },
		{ "controlPressed", &Palette::controlPressed },
		{ "controlBorder", &Palette::controlBorder },
		{ "field", &Palette::field },
		{ "track", &Palette::track },
		{ "accent", &Palette::accent },
		{ "accentHovered", &Palette::accentHovered },
		{ "accentPressed", &Palette::accentPressed },
		{ "onAccent", &Palette::onAccent },
		{ "selection", &Palette::selection },
		{ "neutralSelection", &Palette::neutralSelection },
		{ "menuHighlight", &Palette::menuHighlight },
		{ "sliderFill", &Palette::sliderFill },
		{ "toggleOn", &Palette::toggleOn },
		{ "toggleOnHovered", &Palette::toggleOnHovered },
		{ "onToggle", &Palette::onToggle },
		{ "text", &Palette::text },
		{ "mutedText", &Palette::mutedText },
		{ "disabledText", &Palette::disabledText },
		{ "warningText", &Palette::warningText },
		{ "separator", &Palette::separator },
		{ "tooltip", &Palette::tooltip },
		{ "scrim", &Palette::scrim },
		{ "shadow", &Palette::shadow },
		{ "windowCloseHovered", &Palette::windowCloseHovered },
		{ "windowClosePressed", &Palette::windowClosePressed },
		{ "onWindowClose", &Palette::onWindowClose },
	};
	return fields;
}

const std::vector<ThemeMetricField>& tp::ui::themeMetricFields() {
	static const std::vector<ThemeMetricField> fields = {
		{ "fontSize", &Metrics::fontSize, 9, 20 },
		{ "cornerRadius", &Metrics::cornerRadius, 0, 16 },
		{ "popupCornerRadius", &Metrics::popupCornerRadius, 0, 20 },
		{ "surfaceCornerRadius", &Metrics::surfaceCornerRadius, 0, 24 },
		{ "controlHeight", &Metrics::controlHeight, 18, 40 },
		{ "spacing", &Metrics::spacing, 0, 16 },
	};
	return fields;
}

std::string tp::ui::colorHex(const RGBA& color) {
	const auto channel = [](halnf value) { return (int) std::lround(std::fmin(std::fmax(value, 0.0f), 1.0f) * 255); };
	char text[16];
	std::snprintf(
		text, sizeof(text), "#%02X%02X%02X%02X", channel(color.r), channel(color.g), channel(color.b), channel(color.a)
	);
	return text;
}

std::string tp::ui::themeSource(const Theme& theme) {
	std::string source = "Theme theme;\n";
	for (const ThemeColorField& field : themeColorFields()) {
		const RGBA& color = theme.palette.*field.member;
		source += "theme.palette." + std::string(field.name) + " = { " + number(color.r) + ", " + number(color.g) + ", " +
							number(color.b) + ", " + number(color.a) + " };\n";
	}
	for (const ThemeMetricField& field : themeMetricFields())
		source += "theme.metrics." + std::string(field.name) + " = " + number(theme.metrics.*field.member) + ";\n";
	return source;
}

ColorSwatch::ColorSwatch() { setDebugName("color swatch"); }

void ColorSwatch::setColor(const RGBA& color) {
	mColor = color;
	requestPaint();
}

Extent ColorSwatch::measureContent(Axis, halnf) const { return Extent::rigid(kSwatchSize); }

void ColorSwatch::paint(Painter& painter) {
	const RectF box = localRect();
	const halnf radius = theme().metrics.indicatorCornerRadius;
	const halnf half = box.size.x / 2;
	painter.fillRect(box, { 1, 1, 1, 1 }, radius);
	painter.fillRect(makeRect(0, 0, half, half), { 0.75f, 0.75f, 0.75f, 1 }, radius / 2);
	painter.fillRect(makeRect(half, half, half, half), { 0.75f, 0.75f, 0.75f, 1 }, radius / 2);
	painter.fillRect(box, mColor, radius);
	painter.strokeRect(box, theme().palette.controlBorder, theme().metrics.indicatorCornerRadius);
}

ThemeColorRow::ThemeColorRow(const ThemeColorField& field) :
	mField(field) {
	setDebugName(std::string("theme color: ") + field.name);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<RowLayout>(6).setCrossAlign(CrossAlign::Center);

	name.setText(field.name);
	name.setStyle(Button::Style::Flat);
	name.setSizing(AxisSizing::fill(), AxisSizing::fit());
	hex.setTone(Label::Tone::Muted);

	addChild(swatch);
	addChild(name);
	addChild(hex);
}

void ThemeColorRow::refresh(const Theme& theme) {
	const RGBA& color = theme.palette.*mField.member;
	swatch.setColor(color);
	hex.setText(colorHex(color));
}

void ThemeColorRow::setSelected(bool selected) { name.setSelected(selected); }

ThemeEditorPage::ThemeEditorPage() {
	setDebugName("theme editor page");
	RowLayout& layout = emplaceLayout<RowLayout>(16);
	layout.setPadding(Insets::uniform(16));

	colorList.setSizing(AxisSizing::fill(), AxisSizing::fit());
	colorList.emplaceLayout<ColumnLayout>(1);
	for (size_t index = 0; index < themeColorFields().size(); index++) {
		colorRows.push_back(std::make_unique<ThemeColorRow>(themeColorFields()[index]));
		colorRows.back()->name.setOnActivate([this, index]() { selectColor(index); });
		colorRows.back()->swatch.setOnActivate([this, index]() { selectColor(index); });
		colorList.addChild(*colorRows.back());
	}
	colorScroll.setContent(&colorList);
	colorScroll.setSizing(AxisSizing::fill(), AxisSizing::fill());

	mColorColumn.setSizing(AxisSizing::fixed(kColorColumnWidth), AxisSizing::fill());
	mColorColumn.emplaceLayout<ColumnLayout>(6);
	mColorColumn.addChild(colorScroll);

	wheel.setSizing(AxisSizing::fill(), AxisSizing::fixed(200));
	wheel.setOnChanged([this](const RGBA& color) {
		edit([this, &color](Theme& theme) {
			RGBA& target = theme.palette.*themeColorFields()[mSelected].member;
			target = { color.r, color.g, color.b, target.a };
		});
	});
	alpha.setSizing(AxisSizing::fill(), AxisSizing::fit());
	alpha.setValueFormatter(twoDecimals);
	alpha.setOnChanged([this](halnf value) {
		edit([this, value](Theme& theme) { (theme.palette.*themeColorFields()[mSelected].member).a = value; });
	});

	mEditorColumn.setSizing(AxisSizing::fixed(kEditorColumnWidth), AxisSizing::fit());
	mEditorColumn.emplaceLayout<ColumnLayout>(8);
	mEditorColumn.addChild(editingTitle);
	mEditorColumn.addChild(wheel);
	mEditorColumn.addChild(alpha);
	mEditorColumn.addChild(mSeparator);

	for (const ThemeMetricField& field : themeMetricFields()) {
		metricSliders.push_back(std::make_unique<LabeledSlider>(field.name, field.minimum, field.maximum, 0));
		LabeledSlider& slider = *metricSliders.back();
		slider.setSizing(AxisSizing::fill(), AxisSizing::fit());
		slider.setValueFormatter([](halnf value) { return std::to_string((int) std::lround(value)); });
		slider.slider().setStep(1);
		halnf Metrics::* member = field.member;
		slider.setOnChanged([this, member](halnf value) {
			edit([member, value](Theme& theme) { theme.metrics.*member = value; });
		});
		mEditorColumn.addChild(slider);
	}

	accentPreset.setTooltip("Accent preset");
	accentPreset.setOnSelected([this](long index) {
		if (index < 0 || index >= 5) return;
		edit([index](Theme& theme) { theme.palette.setAccent(kAccentPresets[index]); });
	});
	reset.setOnActivate([this]() {
		edit([](Theme& theme) { theme = Theme::standard(); });
		status.setText("Reset to the standard theme");
	});
	copySource.setOnActivate([this]() {
		if (ui() && ui()->windowHost()) ui()->windowHost()->setClipboardText(themeSource(editedTheme()));
		status.setText("Copied theme as C++");
	});
	status.setTone(Label::Tone::Muted);

	mActionRow.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mActionRow.emplaceLayout<RowLayout>(6).setCrossAlign(CrossAlign::Center);
	mActionRow.addChild(accentPreset);
	mActionRow.addChild(reset);
	mActionRow.addChild(copySource);
	mEditorColumn.addChild(mActionRow);
	mEditorColumn.addChild(status);

	addChild(mColorColumn);
	addChild(mEditorColumn);
	refreshEditors();
}

Theme ThemeEditorPage::editedTheme() const { return ui() ? ui()->theme() : Theme::standard(); }

void ThemeEditorPage::onAttached() { refreshEditors(); }

void ThemeEditorPage::onThemeChanged() {
	Widget::onThemeChanged();
	refreshEditors();
}

void ThemeEditorPage::selectColor(size_t index) {
	if (index >= colorRows.size()) return;
	mSelected = index;
	refreshEditors();
}

template <typename Edit>
void ThemeEditorPage::edit(Edit&& change) {
	if (mRefreshing || !ui()) return;

	Theme theme = ui()->theme();
	change(theme);
	ui()->setTheme(theme);
}

void ThemeEditorPage::refreshEditors() {
	const Theme theme = editedTheme();
	mRefreshing = true;

	for (size_t index = 0; index < colorRows.size(); index++) {
		colorRows[index]->refresh(theme);
		colorRows[index]->setSelected(index == mSelected);
	}

	const ThemeColorField& field = themeColorFields()[mSelected];
	const RGBA& color = theme.palette.*field.member;
	editingTitle.setText(std::string("Editing ") + field.name);
	wheel.setColor(color);
	alpha.setValue(color.a);

	for (size_t index = 0; index < metricSliders.size(); index++)
		metricSliders[index]->setValue(theme.metrics.*themeMetricFields()[index].member);

	mRefreshing = false;
}
