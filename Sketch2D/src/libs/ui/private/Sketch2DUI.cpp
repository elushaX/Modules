#include "Sketch2DUI.hpp"
#include "Profiler.hpp"

#include <chrono>
#include <thread>
#include <filesystem>

#include "BoxLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tp::sketch2d {

	using namespace ui;

	namespace {
		constexpr halnf kPopupWidth = 240;
		constexpr halnf kLayersPopupWidth = 280;
		constexpr halnf kThumbnailSize = LayerThumbnail::kSize;
		constexpr halnf kToolStripWidth = 48;

		std::string percentText(halnf fraction) { return std::to_string((int) std::round(fraction * 100)) + "%"; }

		std::string pixelsText(halnf size) { return std::to_string((int) std::round(size)) + " px"; }

		constexpr halnf kPresetIconSize = 28;

		void makePopupPanel(Panel& panel, halnf width) {
			panel.setBorder(true);
			panel.setSizing(AxisSizing::fixed(width), AxisSizing::fit());
			panel.emplaceLayout<ColumnLayout>(8).setPadding(Insets::uniform(10));
		}

		void
		makeToolbarButton(Button& button, const IconProvider& icons, const std::string& icon, const std::string& text) {
			button.setText(text);
			button.setIcon(icons.icon(icon));
			button.setIconPlacement(Button::IconPlacement::Above);
			button.setStyle(Button::Style::Flat);
			button.setSizing(AxisSizing::fit().atLeast(58), AxisSizing::fill());
		}

		void ensureNotErasing(Brush& brush) {
			brush.setEraserMode(false);
			if (brush.type != BrushType::Eraser) return;
			brush.apply(*findBrushPreset(BrushType::Round));
		}
	}

	ColorCompare::ColorCompare() {
		setDebugName("color compare");
		setSizing(AxisSizing::fixed(64), AxisSizing::fixed(28));
	}

	Extent ColorCompare::measureContent(Axis axis, halnf) const {
		const halnf length = axis == Axis::Horizontal ? 64.0f : 28.0f;
		return { length, length, length, 0 };
	}

	RectF ColorCompare::previousRect() const { return makeRect(0, 0, size().x / 2, size().y); }

	RectF ColorCompare::currentRect() const { return makeRect(size().x / 2, 0, size().x / 2, size().y); }

	void ColorCompare::onEvent(Event& event) {
		if (event.type() != Event::Type::PointerDown || event.button() != PointerButton::Primary) return;
		if (!ui::contains(previousRect(), event.position())) return;
		event.accept();
		if (onRestore) onRestore();
	}

	void ColorCompare::paint(Painter& painter) {
		painter.fillRect(previousRect(), previous);
		painter.fillRect(currentRect(), current);
		painter.strokeRect(localRect(), theme().palette.controlBorder, 0, 1);
	}

	ColorPopup::ColorPopup(Brush& brush) :
		mBrush(brush) {
		setDebugName("color popup");
		makePopupPanel(*this, kPopupWidth);

		pages.setSizing(AxisSizing::fill(), AxisSizing::fit());
		wheel.setSizing(AxisSizing::fill(), AxisSizing::fixed(kPickerHeight));
		square.setSizing(AxisSizing::fill(), AxisSizing::fixed(kPickerHeight));
		wheel.setOnChanged([this](const RGBA& color) {
			const HueColor converted = hueColorOf(color);
			const bool gray = converted.saturation <= 1e-6f || converted.value <= 1e-6f;
			pick({ gray ? mPrimary.hueDegrees : converted.hueDegrees, converted.saturation, converted.value });
		});
		square.setOnChanged([this](const HSV& hsv) { pick({ hsv.h * 180.0f / 3.14159265f, hsv.s, hsv.v }); });

		const auto page = [](Widget& page, std::initializer_list<LabeledSlider*> sliders) {
			page.setSizing(AxisSizing::fill(), AxisSizing::fixed(kPickerHeight));
			page.emplaceLayout<ColumnLayout>(10);
			for (LabeledSlider* slider : sliders) {
				slider->setSizing(AxisSizing::fill(), AxisSizing::fit());
				slider->setValueFormatter([](halnf value) { return std::to_string((int) std::round(value)); });
				page.addChild(*slider);
			}
		};
		page(rgbPage, { &red, &green, &blue });
		page(hsvPage, { &hsvHue, &hsvSaturation, &hsvValue });
		page(hslPage, { &hslHue, &hslSaturation, &hslLightness });

		const auto rgbChanged = [this](halnf) {
			if (mShowing) return;
			const HueColor converted =
				hueColorOf(RGBA(red.value() / 255.0f, green.value() / 255.0f, blue.value() / 255.0f, 1.0f));
			const bool gray = converted.saturation <= 1e-6f || converted.value <= 1e-6f;
			pick({ gray ? mPrimary.hueDegrees : converted.hueDegrees, converted.saturation, converted.value });
		};
		for (LabeledSlider* slider : { &red, &green, &blue })
			slider->setOnChanged(rgbChanged);
		const auto hsvChanged = [this](halnf) {
			if (!mShowing) pick({ hsvHue.value(), hsvSaturation.value() / 100.0f, hsvValue.value() / 100.0f });
		};
		for (LabeledSlider* slider : { &hsvHue, &hsvSaturation, &hsvValue })
			slider->setOnChanged(hsvChanged);
		const auto hslChanged = [this](halnf) {
			if (mShowing) return;
			pick(hueColorOf(HslColor{ hslHue.value(), hslSaturation.value() / 100.0f, hslLightness.value() / 100.0f }));
		};
		for (LabeledSlider* slider : { &hslHue, &hslSaturation, &hslLightness })
			slider->setOnChanged(hslChanged);

		red.slider().setTrackGradient([this]() {
			const RGBA c = mPrimary.rgba();
			return std::vector<RGBA>{ { 0, c.g, c.b, 1 }, { 1, c.g, c.b, 1 } };
		});
		green.slider().setTrackGradient([this]() {
			const RGBA c = mPrimary.rgba();
			return std::vector<RGBA>{ { c.r, 0, c.b, 1 }, { c.r, 1, c.b, 1 } };
		});
		blue.slider().setTrackGradient([this]() {
			const RGBA c = mPrimary.rgba();
			return std::vector<RGBA>{ { c.r, c.g, 0, 1 }, { c.r, c.g, 1, 1 } };
		});
		const auto hues = [](halnf saturation, halnf value) {
			std::vector<RGBA> stops;
			for (int4 step = 0; step <= 6; step++)
				stops.push_back(HueColor{ 60.0f * (halnf) step, saturation, value }.rgba());
			return stops;
		};
		hsvHue.slider().setTrackGradient([hues]() { return hues(1, 1); });
		hslHue.slider().setTrackGradient([hues]() { return hues(1, 1); });
		hsvSaturation.slider().setTrackGradient([this]() {
			return std::vector<RGBA>{ HueColor{ mPrimary.hueDegrees, 0, mPrimary.value }.rgba(),
																HueColor{ mPrimary.hueDegrees, 1, mPrimary.value }.rgba() };
		});
		hsvValue.slider().setTrackGradient([this]() {
			return std::vector<RGBA>{ HueColor{ mPrimary.hueDegrees, mPrimary.saturation, 0 }.rgba(),
																HueColor{ mPrimary.hueDegrees, mPrimary.saturation, 1 }.rgba() };
		});
		hslSaturation.slider().setTrackGradient([this]() {
			const HslColor hsl = hslOf(mPrimary);
			return std::vector<RGBA>{ hueColorOf(HslColor{ hsl.hueDegrees, 0, hsl.lightness }).rgba(),
																hueColorOf(HslColor{ hsl.hueDegrees, 1, hsl.lightness }).rgba() };
		});
		hslLightness.slider().setTrackGradient([this]() {
			const HslColor hsl = hslOf(mPrimary);
			return std::vector<RGBA>{ RGBA(0, 0, 0, 1),
																hueColorOf(HslColor{ hsl.hueDegrees, hsl.saturation, 0.5f }).rgba(),
																RGBA(1, 1, 1, 1) };
		});

		pages.addTab("Wheel", wheel);
		pages.addTab("Square", square);
		pages.addTab("RGB", rgbPage);
		pages.addTab("HSV", hsvPage);
		pages.addTab("HSL", hslPage);

		history.setSizing(AxisSizing::fill(), AxisSizing::fit());
		history.setOnPicked([this](size_t index) {
			if (index < mHistoryColors.size()) pick(mHistoryColors[index]);
		});
		historyTitle.setTone(Label::Tone::Muted);
		swatches.setSizing(AxisSizing::fill(), AxisSizing::fit());
		swatches.setOnPicked([this](size_t index) {
			if (index < swatches.colors().size()) pick(hueColorOf(swatches.colors()[index]));
		});
		swatches.setOnHeld([this](size_t index) {
			if (onRemoveSwatch) onRemoveSwatch(index);
		});
		swatchHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
		swatchHeader.emplaceLayout<RowLayout>(6).setCrossAlign(CrossAlign::Center);
		swatchTitle.setTone(Label::Tone::Muted);
		swatchTitle.setSizing(AxisSizing::fill(), AxisSizing::fit());
		addSwatch.setStyle(Button::Style::Flat);
		addSwatch.setTooltip("Add the current color to swatches");
		addSwatch.setOnActivate([this]() {
			if (onAddSwatch) onAddSwatch();
		});
		swatchHeader.addChild(swatchTitle);
		swatchHeader.addChild(addSwatch);
		swatchHint.setTone(Label::Tone::Muted);
		swatchHint.setWrap(true);
		swatchHint.setSizing(AxisSizing::fill(), AxisSizing::fit());

		footer.setSizing(AxisSizing::fill(), AxisSizing::fit());
		footer.emplaceLayout<RowLayout>(6);
		hex.setSizing(AxisSizing::fill(), AxisSizing::fit());
		hex.setMaximumLength(7);
		hex.setOnCommitted([this](const std::string& text) {
			if (const auto parsed = parseColorHex(text)) {
				const HueColor converted = hueColorOf(*parsed);
				const bool gray = converted.saturation <= 1e-6f || converted.value <= 1e-6f;
				pick({ gray ? mPrimary.hueDegrees : converted.hueDegrees, converted.saturation, converted.value });
			}
			hex.setText(colorHex(mPrimary.rgba()));
		});
		compare.onRestore = [this]() { pick(hueColorOf(compare.previous)); };
		copy.setStyle(Button::Style::Flat);
		copy.setOnActivate([this]() {
			if (onCopy) onCopy();
		});
		footer.addChild(compare);
		footer.addChild(hex);
		footer.addChild(copy);

		eraserMode.setSizing(AxisSizing::fill(), AxisSizing::fit());
		eraserMode.setOnChanged([this](bool checked) {
			if (!checked) {
				ensureNotErasing(mBrush);
				return;
			}
			mBrush.setEraserMode(true);
		});

		addChild(pages);
		addChild(footer);
		addChild(historyTitle);
		addChild(history);
		addChild(swatchHeader);
		addChild(swatches);
		addChild(swatchHint);
		addChild(eraserMode);
		mPrimary = hueColorOf(brush.color);
		showHistory(ColorHistory());
		showSwatches(SwatchSet());
		markPrevious();
		refresh();
	}

	void ColorPopup::pick(const HueColor& color) {
		mPrimary = color;
		mBrush.color = color.rgba();
		ensureNotErasing(mBrush);
		showColor();
		if (onPicked) onPicked(color);
	}

	void ColorPopup::setPrimary(const HueColor& color) {
		mPrimary = color;
		showColor();
	}

	void ColorPopup::markPrevious() { compare.previous = mPrimary.rgba(); }

	void ColorPopup::showHistory(const ColorHistory& entries) {
		mHistoryColors.clear();
		std::vector<RGBA> colors;
		std::vector<std::string> captions;
		for (const ColorHistory::Entry& entry : entries.entries()) {
			mHistoryColors.push_back(entry.color);
			colors.push_back(entry.color.rgba());
			captions.push_back(std::to_string(entry.strokes));
		}
		const bool empty = colors.empty();
		history.setColors(std::move(colors));
		history.setCaptions(std::move(captions));
		historyTitle.setVisible(!empty);
		history.setVisible(!empty);
	}

	void ColorPopup::showSwatches(const SwatchSet& set) {
		swatches.setColors(set.colors());
		swatches.setVisible(!set.colors().empty());
		swatchHint.setText(set.colors().empty() ? "Tap + to keep the current color here" : "Hold a swatch to remove it");
	}

	void ColorPopup::showColor() {
		mShowing = true;
		const RGBA rgb = mPrimary.rgba();
		if (!wheel.isDragging()) wheel.setColor(rgb);
		square.setHsv(HSV(mPrimary.hueDegrees * 3.14159265f / 180.0f, mPrimary.saturation, mPrimary.value));
		red.setValue(std::round(rgb.r * 255));
		green.setValue(std::round(rgb.g * 255));
		blue.setValue(std::round(rgb.b * 255));
		hsvHue.setValue(mPrimary.hueDegrees);
		hsvSaturation.setValue(mPrimary.saturation * 100);
		hsvValue.setValue(mPrimary.value * 100);
		const HslColor hsl = hslOf(mPrimary);
		hslHue.setValue(hsl.hueDegrees);
		hslSaturation.setValue(hsl.saturation * 100);
		hslLightness.setValue(hsl.lightness * 100);
		if (!hex.hasKeyboardFocus()) hex.setText(colorHex(rgb));
		compare.current = rgb;
		compare.requestPaint();
		mShowing = false;
	}

	void ColorPopup::refresh() {
		eraserMode.setVisible(!mBrush.smudges());
		eraserMode.setChecked(mBrush.erases());
		const RGBA shown = mPrimary.rgba();
		const RGBA brush = mBrush.color;
		if (std::fabs(shown.r - brush.r) + std::fabs(shown.g - brush.g) + std::fabs(shown.b - brush.b) > 1e-4f) {
			const HueColor converted = hueColorOf(brush);
			const bool gray = converted.saturation <= 1e-6f || converted.value <= 1e-6f;
			mPrimary = { gray ? mPrimary.hueDegrees : converted.hueDegrees, converted.saturation, converted.value };
		}
		showColor();
	}

	BrushPopup::BrushPopup(Brush& brush) :
		mBrush(brush) {
		setDebugName("brush popup");
		makePopupPanel(*this, kPopupWidth);

		size.setValueFormatter(pixelsText);
		size.setScale(Slider::Scale::Logarithmic);
		flow.setValueFormatter(percentText);
		opacity.setValueFormatter(percentText);
		hardness.setValueFormatter(percentText);

		size.setOnChanged([this](halnf value) {
			mBrush.setSize(value);
			refreshSummaries();
		});
		flow.setOnChanged([this](halnf value) {
			mBrush.setFlow(value);
			refreshSummaries();
		});
		opacity.setOnChanged([this](halnf value) {
			mBrush.setOpacity(value);
			refreshSummaries();
		});
		hardness.setOnChanged([this](halnf value) {
			mBrush.setHardness(value);
			refreshSummaries();
		});
		roundness.setValueFormatter(percentText);
		angle.setValueFormatter([](halnf value) { return std::to_string((int) std::round(value)) + "°"; });
		spacing.setValueFormatter(percentText);
		roundness.setOnChanged([this](halnf value) {
			mBrush.set(BrushParameter::Roundness, value);
			refreshSummaries();
		});
		angle.setOnChanged([this](halnf value) {
			mBrush.set(BrushParameter::Angle, value);
			refreshSummaries();
		});
		spacing.setOnChanged([this](halnf value) {
			mBrush.setSpacing(value);
			refreshSummaries();
		});
		texture.setValueFormatter(percentText);
		texture.setOnChanged([this](halnf value) {
			mBrush.set(BrushParameter::Texture, value);
			refreshSummaries();
		});
		grainScale.setOnChanged([this](halnf value) {
			mBrush.set(BrushParameter::GrainScale, value);
			refreshSummaries();
		});
		stabilizer.setValueFormatter(percentText);
		stabilizer.setOnChanged([this](halnf value) {
			mBrush.setStabilizer(value);
			refreshSummaries();
		});

		for (LabeledSlider* basic : { &size, &opacity, &flow, &hardness, &stabilizer }) {
			basic->setSizing(AxisSizing::fill(), AxisSizing::fit());
			addChild(*basic);
		}

		const std::pair<Collapsible*, std::vector<Widget*>> sections[] = {
			{ &shapeSection, { &roundness, &angle, &spacing, &markerBlend } },
			{ &textureSection, { &texture, &grainScale } },
			{ &pressureSection, { &sizePressure, &opacityPressure, &flowPressure } },
			{ &smudgeSection, { &smudgeAmount, &blur, &resaturation, &linearMixing } },
		};
		for (const auto& [section, members] : sections) {
			for (Widget* member : members) {
				member->setSizing(AxisSizing::fill(), AxisSizing::fit());
				section->body().addChild(*member);
			}
			addChild(*section);
		}
		reset.setStyle(Button::Style::Flat);
		reset.setSizing(AxisSizing::fill(), AxisSizing::fit());
		reset.setOnActivate([this]() {
			mBrush.resetEverything();
			refresh();
			if (onReset) onReset();
		});
		addChild(reset);

		const std::pair<Checkbox*, PressureTarget> toggles[] = {
			{ &sizePressure, PressureTarget::Size },
			{ &opacityPressure, PressureTarget::Opacity },
			{ &flowPressure, PressureTarget::Flow },
		};
		smudgeAmount.setValueFormatter(percentText);
		resaturation.setValueFormatter([](halnf value) { return std::to_string((int) std::round(value * 1000)) + "‰"; });
		const std::pair<LabeledSlider*, BrushParameter> smudgeSliders[] = {
			{ &smudgeAmount, BrushParameter::Smudge },
			{ &blur, BrushParameter::Blur },
			{ &resaturation, BrushParameter::Resaturation },
		};
		for (const auto& [slider, parameter] : smudgeSliders) {
			slider->setOnChanged([this, parameter](halnf value) {
				mBrush.set(parameter, value);
				refreshSummaries();
			});
		}
		linearMixing.setOnChanged([this](bool checked) {
			mBrush.setLinearMixing(checked);
			refreshSummaries();
		});
		markerBlend.setOnChanged([this](bool checked) {
			mBrush.setMarkerBlend(checked);
			refreshSummaries();
		});
		for (const auto& [box, target] : toggles) {
			box->setOnChanged([this, target](bool checked) {
				mBrush.setUsesPressure(target, checked);
				refreshSummaries();
			});
		}
		refresh();
	}

	void BrushPopup::refresh() {
		size.setValue(mBrush.size());
		flow.setValue(mBrush.flow());
		opacity.setValue(mBrush.opacity());
		hardness.setValue(mBrush.hardness());
		roundness.setValue(mBrush.get(BrushParameter::Roundness));
		angle.setValue(mBrush.get(BrushParameter::Angle));
		spacing.setValue(mBrush.spacing());
		texture.setValue(mBrush.get(BrushParameter::Texture));
		grainScale.setValue(mBrush.get(BrushParameter::GrainScale));
		stabilizer.setValue(mBrush.stabilizer());
		sizePressure.setChecked(mBrush.usesPressure(PressureTarget::Size));
		opacityPressure.setChecked(mBrush.usesPressure(PressureTarget::Opacity));
		flowPressure.setChecked(mBrush.usesPressure(PressureTarget::Flow));
		markerBlend.setChecked(mBrush.markerBlend());
		smudgeAmount.setValue(mBrush.get(BrushParameter::Smudge));
		blur.setValue(mBrush.get(BrushParameter::Blur));
		resaturation.setValue(mBrush.get(BrushParameter::Resaturation));
		linearMixing.setChecked(mBrush.linearMixing());
		smudgeSection.setVisible(mBrush.smudges());
		markerBlend.setVisible(!mBrush.smudges());
		refreshSummaries();
	}

	void BrushPopup::refreshSummaries() {
		shapeSection.header().setSummary(
			percentText(mBrush.get(BrushParameter::Roundness)) + "  " +
			std::to_string((int) std::round(mBrush.get(BrushParameter::Angle))) + "°" +
			(mBrush.markerBlend() ? "  marker" : "")
		);
		textureSection.header().setSummary(percentText(mBrush.get(BrushParameter::Texture)));
		smudgeSection.header().setSummary(
			percentText(mBrush.get(BrushParameter::Smudge)) + "  blur " +
			std::to_string((int) std::round(mBrush.get(BrushParameter::Blur))) + (mBrush.linearMixing() ? "  linear" : "")
		);
		std::string pressure;
		if (mBrush.usesPressure(PressureTarget::Size)) pressure += "size ";
		if (mBrush.usesPressure(PressureTarget::Opacity)) pressure += "opacity ";
		if (mBrush.usesPressure(PressureTarget::Flow)) pressure += "flow ";
		pressureSection.header().setSummary(
			pressure.empty() ? std::string("off") : pressure.substr(0, pressure.size() - 1)
		);
	}

	BrushPresetButton::BrushPresetButton(const BrushPreset& preset) :
		Button(preset.name),
		mPreset(preset) {
		setDebugName(std::string("brush preset ") + preset.name);
		setStyle(Button::Style::Flat);
		setSizing(AxisSizing::fill(), AxisSizing::fit());
		setIconSize(kPresetIconSize);
	}

	std::vector<Vec2F> BrushPresetButton::samplePath(const RectF& box) const {
		std::vector<Vec2F> points;
		constexpr int kSteps = 16;
		for (int step = 0; step <= kSteps; step++) {
			const halnf amount = (halnf) step / kSteps;
			const halnf x = box.pos.x + box.size.x * (0.1f + 0.8f * amount);
			const halnf y = box.pos.y + box.size.y * (0.5f - 0.28f * std::sin(amount * 6.2831853f));
			points.emplace_back(x, y);
		}
		return points;
	}

	void BrushPresetButton::paintIcon(Painter& painter, const RectF& box) {
		const RGBA ink = isEffectivelyEnabled() ? theme().palette.text : theme().palette.disabledText;
		const std::vector<Vec2F> path = samplePath(box);
		const halnf unit = box.size.y / 24;
		const auto faded = [&ink](halnf alpha) { return RGBA{ ink.r, ink.g, ink.b, ink.a * alpha }; };
		switch (mPreset.type) {
			case BrushType::Pencil: painter.polyline(path, faded(0.85f), 1.2f * unit); return;
			case BrushType::Ink: painter.polyline(path, ink, 2.0f * unit); return;
			case BrushType::Marker: painter.polyline(path, faded(0.55f), 5.0f * unit); return;
			case BrushType::Round: painter.polyline(path, ink, 3.5f * unit); return;
			case BrushType::Flat:
				for (const Vec2F& point : path)
					painter.fillRect(makeRect(point - Vec2F(0.6f, 3.0f) * unit, Vec2F(1.2f, 6.0f) * unit), faded(0.8f));
				return;
			case BrushType::Calligraphy:
				for (const Vec2F& point : path)
					painter.line(point - Vec2F(1.6f, -1.6f) * unit, point + Vec2F(1.6f, -1.6f) * unit, ink, 1.4f * unit);
				return;
			case BrushType::Air:
				for (const Vec2F& point : path)
					painter.fillCircle(point, 3.2f * unit, faded(0.18f));
				return;
			case BrushType::Jagged:
				for (size_t index = 0; index < path.size(); index++) {
					const halnf jitter = (index % 2 ? 1.0f : -1.0f) * 1.6f * unit;
					painter.fillCircle(path[index] + Vec2F(0.0f, jitter), 1.6f * unit, ink);
				}
				return;
			case BrushType::Smudge:
				painter.polyline(path, faded(0.25f), 5.0f * unit);
				painter.polyline(path, faded(0.6f), 1.5f * unit);
				return;
			case BrushType::Eraser: painter.polyline(path, theme().palette.disabledText, 3.5f * unit); return;
		}
	}

	BrushListPopup::BrushListPopup(Brush& brush) :
		mBrush(brush) {
		setDebugName("brush list popup");
		makePopupPanel(*this, kPopupWidth);
		for (const BrushPreset& preset : brushPresets()) {
			if (preset.type == BrushType::Eraser) continue;
			auto button = std::make_unique<BrushPresetButton>(preset);
			const BrushPreset* chosen = &preset;
			button->setOnActivate([this, chosen]() {
				mBrush.apply(*chosen);
				refresh();
				if (onChosen) onChosen();
			});
			addChild(*button);
			mButtons.push_back(std::move(button));
		}
		refresh();
	}

	void BrushListPopup::refresh() {
		for (const auto& button : mButtons)
			button->setSelected(button->text() == mBrush.presetName());
	}

	Button& BrushListPopup::button(const std::string& name) {
		for (const auto& button : mButtons) {
			if (button->text() == name) return *button;
		}
		return *mButtons.front();
	}

	LayerRow::LayerRow(
		Document& document, Layer& layer, const IconProvider& icons, const std::function<void()>& onChanged
	) :
		mDocument(document),
		mLayer(layer) {
		setDebugName("layer row: " + layer.name());
		setSizing(AxisSizing::fill(), AxisSizing::fit());
		emplaceLayout<RowLayout>(4).setCrossAlign(CrossAlign::Center);

		visibility.setIcon(icons.icon("view"));
		visibility.setStyle(Button::Style::Flat);
		visibility.setSelected(layer.visible());
		visibility.setTooltip("Visible");
		visibility.setOnActivate([this, onChanged]() {
			mDocument.setLayerVisible(mLayer, !mLayer.visible());
			mDocument.commit(commandInfo(Command::ToggleLayerVisible).history);
			onChanged();
		});

		thumbnail.setSizing(AxisSizing::fixed(kThumbnailSize), AxisSizing::fixed(kThumbnailSize));

		name.setText(layer.name());
		name.setStyle(Button::Style::Flat);
		name.setSelected(document.activeLayer() == &layer || layer.selected());
		name.setSizing(AxisSizing::fill(), AxisSizing::fit());
		name.setOnActivate([this, onChanged]() {
			select(Modifiers{});
			onChanged();
		});
		name.setOnClicked([this, onChanged](const Event& click) {
			select(click.modifiers());
			const Modifiers& modifiers = click.modifiers();
			const bool plain = !modifiers.control && !modifiers.shift && !modifiers.alt;
			if (plain && click.clickCount() == 2 && onRename) onRename(mLayer);
			onChanged();
		});

		details.setText(detailText());
		details.setTone(Label::Tone::Muted);
		details.setFontSize(Theme::standard().metrics.smallFontSize);

		mText.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mText.emplaceLayout<ColumnLayout>(0);
		mText.addChild(name);
		mText.addChild(details);

		remove.setStyle(Button::Style::Flat);
		remove.setTooltip("Delete");
		remove.setOnActivate([this, onChanged]() {
			mDocument.setActiveLayer(&mLayer);
			if (mLayer.locked()) return;
			mDocument.removeActiveLayer();
			mDocument.commit(commandInfo(Command::DeleteLayer).history);
			onChanged();
		});

		addChild(visibility);
		addChild(thumbnail);
		addChild(mText);
		addChild(remove);
	}

	std::string LayerRow::detailText() const {
		std::string text = blendModeName(mLayer.blend());
		text += " " + percentText(mLayer.opacity());
		if (mLayer.clipped()) text += ", Clipping mask";
		if (mLayer.locked()) text += ", Locked";
		if (mLayer.alphaLocked()) text += ", Locked alpha";
		return text;
	}

	void LayerRow::setThumbnail(ImageId image) {
		mThumbnail = image;
		requestPaint();
	}

	void LayerRow::paint(Painter& painter) {
		Widget::paint(painter);

		const RectF box = thumbnail.rect();
		const Palette& palette = theme().palette;
		const halnf cell = box.size.x / 4;
		for (int4 y = 0; y < 4; y++) {
			for (int4 x = 0; x < 4; x++) {
				const bool light = (x + y) % 2 == 0;
				painter.fillRect(
					makeRect(box.pos.x + (halnf) x * cell, box.pos.y + (halnf) y * cell, cell, cell),
					light ? palette.mutedText : palette.disabledText
				);
			}
		}
		if (mThumbnail.isValid()) painter.image(box, mThumbnail);
		painter.strokeRect(box, palette.border, 0, 1);
	}

	void LayerRow::select(const Modifiers& modifiers) {
		bool changed = false;
		if (modifiers.control) changed = mDocument.toggleLayerSelection(mLayer);
		else if (modifiers.shift) changed = mDocument.selectRangeTo(mLayer);
		else changed = mDocument.selectOnly(mLayer);
		if (changed) mDocument.commit(commandInfo(Command::SelectLayerAbove).history, "layer_select");
	}

	LayersPopup::LayersPopup(Document& document, const IconProvider& icons) :
		mDocument(document),
		mIcons(icons) {
		setDebugName("layers popup");
		makePopupPanel(*this, kLayersPopupWidth);

		add.setText(commandInfo(Command::NewLayer).label);
		add.setOnActivate([this]() {
			if (!mDocument.addLayer(mDocument.nextLayerName())) return;
			mDocument.commit(commandInfo(Command::NewLayer).history);
			changed();
		});

		std::vector<std::string> modes;
		for (int4 mode = 0; mode < kBlendModeCount; mode++)
			modes.push_back(blendModeName((BlendMode) mode));
		blend.setOptions(modes);
		blend.setTooltip("Blend mode");
		blend.setSizing(AxisSizing::fill(), AxisSizing::fit());
		blend.setOnSelected([this](long index) {
			if (mDocument.setBlend((BlendMode) index)) {
				mDocument.commit(commandInfo(Command::NormalLayerBlend).history, "layer_blend");
				changed();
			}
		});

		opacity.setValueFormatter(percentText);
		opacity.setOnChanged([this](halnf value) {
			if (mDocument.setOpacity(value)) {
				mDocument.commit(commandInfo(Command::LayerOpacityIncrease).history, "layer_opacity");
				refreshRows();
				if (onChanged) onChanged();
			}
		});

		mList.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mList.setOnMoved([this](size_t from, size_t to) { moveRow(from, to); });

		addChild(mTitle);
		addChild(blend);
		addChild(opacity);
		addChild(mList);
		addChild(add);
		refresh();
	}

	void LayersPopup::changed() {
		refresh();
		if (onChanged) onChanged();
	}

	void LayersPopup::refresh() {
		mList.clearItems();
		mRows.clear();

		const auto& layers = mDocument.layers();
		for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
			auto row = std::make_unique<LayerRow>(mDocument, **it, mIcons, [this]() { changed(); });
			row->onRename = [this](Layer& layer) {
				if (onRename) onRename(layer);
			};
			mRows.push_back(std::move(row));
			mList.addItem(*mRows.back());
		}

		refreshRows();
	}

	void LayersPopup::showThrough(PopupButton& button) {
		mHosts.push_back(&button);
		mShownBy = [this]() {
			for (PopupButton* host : mHosts)
				if (host->isPopupOpen()) return true;
			return false;
		};
		button.setOnOpening([this]() { opened(); });
	}

	void LayersPopup::refreshRows() {
		const bool shown = mOpening || !mShownBy || mShownBy();
		mThumbnailsStale = !shown;
		for (const auto& row : mRows) {
			row->details.setText(row->detailText());
			if (mThumbnails && shown) row->setThumbnail(mThumbnails->thumbnail(mDocument, row->layer()));
		}

		Layer* active = mDocument.activeLayer();
		if (!active) return;
		opacity.setValue(active->opacity());
		blend.setSelectedIndex((long) active->blend());
		add.setEnabled(mDocument.canAddLayer());
	}

	void LayersPopup::moveRow(size_t from, size_t to) {
		Layer& layer = mRows[from]->layer();
		mDocument.setActiveLayer(&layer);
		mDocument.moveActiveLayer((int4) from - (int4) to);
		mDocument.commit(commandInfo(Command::MoveLayerUp).history);
		changed();
	}

	ToolBar::ToolBar(Document& document, Brush& brush, const IconProvider& icons) :
		Panel(Surface::Base),
		colorPopup(brush),
		brushPopup(brush),
		brushListPopup(brush),
		layersPopup(document, icons),
		mDocument(document),
		mBrush(brush) {
		setDebugName("tool bar");
		setCornerRadius(0);
		setSizing(AxisSizing::fill(), AxisSizing::fixed(60));
		emplaceLayout<RowLayout>(2).setPadding(Insets::symmetric(4, 2));

		makeToolbarButton(tool, icons, "tool", "Tool");
		makeToolbarButton(line, icons, "line", "Line");
		makeToolbarButton(brushTool, icons, "brush", "Brush");
		makeToolbarButton(color, icons, "color", "Color");
		makeToolbarButton(brushSize, icons, "size", "");
		makeToolbarButton(zoom, icons, "zoom", "");
		makeToolbarButton(undo, icons, "undo", "Undo");
		makeToolbarButton(redo, icons, "redo", "Redo");
		makeToolbarButton(select, icons, "select", "Select");
		makeToolbarButton(view, icons, "view", "View");
		makeToolbarButton(layers, icons, "layers", "Layers");

		tool.setOnActivate([this]() { ensureNotErasing(mBrush); });
		brushTool.setPopupContent(&brushListPopup);
		brushListPopup.onChosen = [this]() {
			brushPopup.refresh();
			refreshFromState();
		};
		zoom.setOnActivate([this]() { mDocument.setZoom(1.0f); });
		view.setOnActivate([this]() { mDocument.resetView(); });

		color.setPopupContent(&colorPopup);
		brushSize.setPopupContent(&brushPopup);
		layers.setPopupContent(&layersPopup);
		layersPopup.showThrough(layers);
		layers.popupOptions().align = PlacementAlign::End;

		zoom.setTooltip("Reset zoom to 100%");
		view.setTooltip("Reset view");

		mSpacer.setSizing(AxisSizing::fill(), AxisSizing::fill());

		for (Widget* item : std::initializer_list<Widget*>{ &tool, &line, &brushTool, &color, &brushSize, &zoom })
			addChild(*item);
		addChild(mSpacer);
		for (Widget* item : std::initializer_list<Widget*>{ &undo, &redo, &select, &view, &layers })
			addChild(*item);

		refreshFromState();
	}

	void ToolBar::refreshFromState() {
		brushSize.setText(pixelsText(mBrush.size()));
		brushTool.setText(mBrush.type == BrushType::Eraser || mBrush.presetName().empty() ? "Brush" : mBrush.presetName());
		brushListPopup.refresh();
		if (!(mShownBrush == mBrush)) {
			mShownBrush = mBrush;
			brushPopup.refresh();
			colorPopup.refresh();
		}
		zoom.setText(percentText(mDocument.zoom()));
	}

	ToolStrip::ToolStrip(Brush& brush, const IconProvider& icons) :
		Panel(Surface::Base),
		mBrush(brush),
		mIcons(icons) {
		setDebugName("tool strip");
		setCornerRadius(0);
		setSizing(AxisSizing::fixed(kToolStripWidth), AxisSizing::fill());
		emplaceLayout<ColumnLayout>(0);
		mScroller.setSizing(AxisSizing::fill(), AxisSizing::fill());
		mScroller.setShowsScrollBar(false);
		mColumn.emplaceLayout<ColumnLayout>(2).setPadding(Insets::uniform(3));
		mColumn.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mScroller.setContent(&mColumn);
		addChild(mScroller);

		mPaintingBrush.apply(*findBrushPreset(BrushType::Round));
		const auto use = [this](CanvasTool tool) {
			if (setCanvasTool) setCanvasTool(tool);
		};
		addTool("brush", [this, use]() {
			use(CanvasTool::Paint);
			if (isPaintingBrush(mBrush)) return;
			const RGBA color = mBrush.color;
			mBrush = mPaintingBrush;
			mBrush.color = color;
		});
		addTool("eraser", [this, use]() {
			use(CanvasTool::Paint);
			switchToToolBrush(BrushType::Eraser);
		});
		addTool("smudge", [this, use]() {
			use(CanvasTool::Paint);
			switchToToolBrush(BrushType::Smudge);
		});
		addTool("eyedropper", [use]() { use(CanvasTool::Picker); });
		addTool("line", {});
		addTool("transform", [use]() { use(CanvasTool::Transform); });
		addTool("crop", [use]() { use(CanvasTool::Crop); });
		addTool("move", [use]() { use(CanvasTool::Move); });
		addTool("wand", [use]() { use(CanvasTool::Wand); });
		addTool("lasso", [use]() { use(CanvasTool::Lasso); });
		addTool("rectselect", [use]() { use(CanvasTool::Marquee); });
		addTool("gradient", [this, use]() {
			use(CanvasTool::Gradient);
			ensureNotErasing(mBrush);
		});
		addTool("fill", [this, use]() {
			use(CanvasTool::Bucket);
			ensureNotErasing(mBrush);
			if (!isPaintingBrush(mBrush)) {
				const RGBA color = mBrush.color;
				mBrush = mPaintingBrush;
				mBrush.color = color;
			}
		});
		for (const char* placeholder : { "export", "history" })
			addTool(placeholder, {});
		addTool("hand", [use]() { use(CanvasTool::Hand); });
		addTool("zoom", [use]() { use(CanvasTool::Zoom); });
		addTool("rotate", [use]() { use(CanvasTool::Rotate); });

		refreshFromState();
	}

	TransformBar::TransformBar() :
		Panel(Surface::Raised) {
		setDebugName("transform bar");
		setCornerRadius(0);
		setSizing(AxisSizing::fill(), AxisSizing::fit());
		RowLayout& row = emplaceLayout<RowLayout>(6);
		row.setPadding(Insets::symmetric(8, 4));
		row.setCrossAlign(CrossAlign::Center);
		title.setSizing(AxisSizing::fill(), AxisSizing::fit());
		apply.setStyle(Button::Style::Accent);
		proportional.setTooltip("Keep proportions while scaling (Shift)");
		for (Widget* item : std::initializer_list<Widget*>{ &title, &flip, &flop, &proportional, &cancel, &apply })
			addChild(*item);
	}

	bool ToolStrip::isPaintingBrush(const Brush& brush) {
		return brush.type != BrushType::Eraser && brush.type != BrushType::Smudge;
	}

	void ToolStrip::resetBrushes() { mPaintingBrush.apply(*findBrushPreset(BrushType::Round)); }

	void ToolStrip::switchToToolBrush(BrushType tool) {
		if (mBrush.type == tool) return;
		if (isPaintingBrush(mBrush)) mPaintingBrush = mBrush;
		mBrush.apply(*findBrushPreset(tool));
	}

	void ToolStrip::refreshFromState() {
		const CanvasTool tool = canvasTool ? canvasTool() : CanvasTool::Paint;
		const bool painting = tool == CanvasTool::Paint;
		button("brush").setSelected(painting && isPaintingBrush(mBrush));
		button("eraser").setSelected(painting && mBrush.type == BrushType::Eraser);
		button("smudge").setSelected(painting && mBrush.type == BrushType::Smudge);
		button("eyedropper").setSelected(tool == CanvasTool::Picker);
		button("hand").setSelected(tool == CanvasTool::Hand);
		button("zoom").setSelected(tool == CanvasTool::Zoom);
		button("rotate").setSelected(tool == CanvasTool::Rotate);
		button("fill").setSelected(tool == CanvasTool::Bucket);
		button("gradient").setSelected(tool == CanvasTool::Gradient);
		button("move").setSelected(tool == CanvasTool::Move);
		button("transform").setSelected(tool == CanvasTool::Transform);
		button("crop").setSelected(tool == CanvasTool::Crop);
		button("lasso").setSelected(tool == CanvasTool::Lasso);
		button("wand").setSelected(tool == CanvasTool::Wand);
		button("rectselect").setSelected(tool == CanvasTool::Marquee);
	}

	void ToolStrip::reveal(const std::string& icon) {
		const RectF target = button(icon).rect();
		const halnf view = mScroller.viewportLength();
		const halnf offset = mScroller.offset();
		if (target.pos.y < offset) mScroller.jumpTo(target.pos.y);
		else if (target.pos.y + target.size.y > offset + view) mScroller.jumpTo(target.pos.y + target.size.y - view);
	}

	Button& ToolStrip::button(const std::string& icon) {
		for (auto& tool : mTools) {
			if (tool.first == icon) return *tool.second;
		}
		return *mTools.front().second;
	}

	Button& ToolStrip::addTool(const std::string& icon, const std::function<void()>& action) {
		auto button = std::make_unique<Button>();
		button->setIcon(mIcons.icon(icon));
		button->setStyle(Button::Style::Flat);
		button->setTooltip(icon);
		button->setSizing(AxisSizing::fill(), AxisSizing::fixed(kToolStripWidth - 6));
		button->setOnActivate([this, action]() {
			if (action) action();
			refreshFromState();
		});

		mColumn.addChild(*button);
		mTools.emplace_back(icon, std::move(button));
		return *mTools.back().second;
	}

	HistoryLimitDialog::HistoryLimitDialog(Document& document) :
		Dialog("History limit"),
		steps(
			(halnf) History<DocumentState>::kMinimumLimit,
			(halnf) History<DocumentState>::kMaximumLimit,
			(halnf) History<DocumentState>::kDefaultLimit
		),
		mDocument(document) {
		setDebugName("history limit dialog");
		body().addChild(stepsLabel);
		body().addChild(steps);
		addButton("Apply", [this]() { mDocument.history().setLimit((int4) steps.value()); }, true);
		addButton("Cancel", {});
	}

	void HistoryLimitDialog::show(ui::Ui& ui) {
		steps.setValue((halnf) mDocument.history().limit());
		open(ui);
	}

	RenameLayerDialog::RenameLayerDialog() :
		Dialog("Rename layer") {
		setDebugName("rename layer dialog");
		name.setMaximumLength(Document::kMaximumLayerNameLength);
		name.setOnCommitted([this](const std::string&) {
			if (isOpen()) accept();
		});
		body().addChild(name);
		addButton(
			"Rename",
			[this]() {
				if (mOnRename) mOnRename(name.text());
			},
			true
		);
		addButton("Cancel", {});
	}

	void
	RenameLayerDialog::show(ui::Ui& ui, const std::string& current, std::function<void(const std::string&)> onRename) {
		mOnRename = std::move(onRename);
		name.setText(current);
		open(ui);
		ui.setKeyboardFocus(&name);
		name.selectAll();
	}

	CanvasBackgroundDialog::CanvasBackgroundDialog() :
		Dialog("Canvas background") {
		setDebugName("canvas background dialog");
		wheel.setSizing(AxisSizing::fill(), AxisSizing::fixed(210));
		wheel.setOnChanged([this](const RGBA&) { preview(); });
		transparent.setOnChanged([this](bool) { preview(); });
		body().addChild(wheel);
		body().addChild(transparent);
		addButton(
			"Apply",
			[this]() {
				mActive = false;
				if (mOnApply) mOnApply(mBefore, chosen());
			},
			true
		);
		addButton("Cancel", [this]() { restore(); });
		setOnCancel([this]() { restore(); });
	}

	void CanvasBackgroundDialog::show(
		ui::Ui& ui,
		const CanvasBackground& current,
		std::function<void(const CanvasBackground&)> onPreview,
		std::function<void(const CanvasBackground&, const CanvasBackground&)> onApply
	) {
		mActive = false;
		mBefore = current;
		mOnPreview = std::move(onPreview);
		mOnApply = std::move(onApply);
		wheel.setColor(current.color.rgba());
		transparent.setChecked(current.transparent);
		mActive = true;
		open(ui);
	}

	void CanvasBackgroundDialog::preview() {
		if (mActive && mOnPreview) mOnPreview(chosen());
	}

	void CanvasBackgroundDialog::restore() {
		if (!mActive) return;
		mActive = false;
		if (mOnPreview) mOnPreview(mBefore);
	}

	CanvasBackground CanvasBackgroundDialog::chosen() const {
		const HSV& hsv = wheel.hsv();
		CanvasBackground background;
		background.color = { (halnf) (hsv.h * 180.0 / PI), hsv.s, hsv.v };
		background.transparent = transparent.isChecked();
		return background;
	}

	CanvasFrameSizeDialog::CanvasFrameSizeDialog() :
		Dialog("Canvas frame size") {
		setDebugName("canvas frame size dialog");
		width.setSuffix(" px");
		height.setSuffix(" px");
		body().addChild(widthLabel);
		body().addChild(width);
		body().addChild(heightLabel);
		body().addChild(height);
		addButton(
			"Apply",
			[this]() {
				if (mOnApply) mOnApply((int4) width.value(), (int4) height.value());
			},
			true
		);
		addButton("Cancel", {});
	}

	void CanvasFrameSizeDialog::show(ui::Ui& ui, const CanvasFrame& frame, std::function<void(int4, int4)> onApply) {
		mOnApply = std::move(onApply);
		width.setValue((halnf) frame.width());
		height.setValue((halnf) frame.height());
		open(ui);
	}

	AdjustmentDialog::AdjustmentDialog(Document& document) :
		Dialog("Adjust"),
		mDocument(document) {
		setDebugName("adjustment dialog");
		for (LabeledSlider* slider : { &first, &second, &third }) {
			slider->setSizing(AxisSizing::fill(), AxisSizing::fit());
			slider->setOnChanged([this](halnf) { preview(); });
			body().addChild(*slider);
		}
		addButton("Apply", [this]() { finish(true); }, true);
		addButton("Cancel", [this]() { finish(false); });
		setOnCancel([this]() { finish(false); });
	}

	void AdjustmentDialog::show(ui::Ui& ui, AdjustmentKind kind, std::function<void(bool)> onClosed) {
		mKind = kind;
		mOnClosed = std::move(onClosed);
		const auto setup = [](
												 LabeledSlider& slider,
												 bool visible,
												 const std::string& name,
												 halnf low,
												 halnf high,
												 halnf value,
												 std::function<std::string(halnf)> format
											 ) {
			slider.setVisible(visible);
			if (!visible) return;
			slider.setName(name);
			slider.slider().setRange(low, high);
			slider.setValue(value);
			slider.setValueFormatter(std::move(format));
		};
		const auto signedPercent = [](halnf value) {
			const int4 percent = (int4) std::lround(value * 100);
			return (percent > 0 ? "+" : "") + std::to_string(percent) + "%";
		};
		const auto degrees = [](halnf value) { return std::to_string((int4) std::lround(value)) + "°"; };
		const auto stops = [](halnf value) {
			char text[16];
			std::snprintf(text, sizeof text, "%+.1f EV", value);
			return std::string(text);
		};
		switch (kind) {
			case AdjustmentKind::BrightnessExposure:
				setTitle("Brightness & exposure");
				setup(first, true, "Brightness", -1, 1, 0, signedPercent);
				setup(second, true, "Exposure", -4, 4, 0, stops);
				setup(third, false, "", 0, 1, 0, {});
				break;
			case AdjustmentKind::HueSaturation:
				setTitle("Hue & saturation");
				setup(first, true, "Hue", -180, 180, 0, degrees);
				setup(second, true, "Saturation", -1, 1, 0, signedPercent);
				setup(third, true, "Lightness", -1, 1, 0, signedPercent);
				break;
			case AdjustmentKind::ColorBalance:
				setTitle("Color balance");
				setup(first, true, "Cyan / Red", -1, 1, 0, signedPercent);
				setup(second, true, "Magenta / Green", -1, 1, 0, signedPercent);
				setup(third, true, "Yellow / Blue", -1, 1, 0, signedPercent);
				break;
			case AdjustmentKind::Colorize:
				setTitle("Colorize");
				setup(first, true, "Hue", 0, 360, 200, degrees);
				setup(second, true, "Saturation", 0, 1, 0.5f, percentText);
				setup(third, true, "Lightness", -1, 1, 0, signedPercent);
				break;
			case AdjustmentKind::Alpha:
				setTitle("Alpha");
				setup(first, true, "Alpha", -1, 1, 0, signedPercent);
				setup(second, false, "", 0, 1, 0, {});
				setup(third, false, "", 0, 1, 0, {});
				break;
		}
		mOpen = true;
		open(ui);
		preview();
	}

	Adjustment AdjustmentDialog::adjustment() const {
		Adjustment adjustment;
		adjustment.kind = mKind;
		switch (mKind) {
			case AdjustmentKind::BrightnessExposure:
				adjustment.brightness = first.value();
				adjustment.exposure = second.value();
				break;
			case AdjustmentKind::HueSaturation:
			case AdjustmentKind::Colorize:
				adjustment.hueDegrees = first.value();
				adjustment.saturation = second.value();
				adjustment.lightness = third.value();
				break;
			case AdjustmentKind::ColorBalance:
				adjustment.red = first.value();
				adjustment.green = second.value();
				adjustment.blue = third.value();
				break;
			case AdjustmentKind::Alpha: adjustment.alpha = first.value(); break;
		}
		return adjustment;
	}

	void AdjustmentDialog::preview() {
		if (mOpen) mDocument.previewAdjustment(adjustment());
	}

	void AdjustmentDialog::finish(bool apply) {
		if (!mOpen) return;
		mOpen = false;
		bool changed = false;
		if (apply) changed = mDocument.finishAdjustment();
		else mDocument.cancelAdjustment();
		if (mOnClosed) mOnClosed(changed);
	}

	CanvasAppearanceDialog::CanvasAppearanceDialog() :
		Dialog("Canvas appearance") {
		setDebugName("canvas appearance dialog");
		boardHue.setValueFormatter([](halnf value) { return std::to_string((int) std::round(value)) + "°"; });
		boardSaturation.setValueFormatter(percentText);
		boardValue.setValueFormatter(percentText);
		for (LabeledSlider* slider : { &boardHue, &boardSaturation, &boardValue }) {
			slider->setSizing(AxisSizing::fill(), AxisSizing::fit());
			slider->setOnChanged([this](halnf) { apply(); });
			body().addChild(*slider);
		}
		shadow.setOnChanged([this](bool) { apply(); });
		body().addChild(shadow);
		viewStiffness.setValueFormatter([](halnf value) {
			return value >= 1.0f ? std::string("Instant") : percentText(value);
		});
		viewStiffness.slider().setStep(0.05f);
		viewStiffness.setSizing(AxisSizing::fill(), AxisSizing::fit());
		viewStiffness.setOnChanged([this](halnf) { apply(); });
		body().addChild(viewStiffness);

		addButton("Done", {}, true);
		addButton("Cancel", [this]() {
			if (mSettings) *mSettings = mBefore;
		});
		setOnCancel([this]() {
			if (mSettings) *mSettings = mBefore;
		});
	}

	void CanvasAppearanceDialog::show(ui::Ui& ui, ViewSettings& settings) {
		mSettings = nullptr;
		mBefore = settings;
		boardHue.setValue(settings.boardColor.hueDegrees);
		boardSaturation.setValue(settings.boardColor.saturation);
		boardValue.setValue(settings.boardColor.value);
		shadow.setChecked(settings.canvasShadow);
		viewStiffness.setValue(settings.viewStiffness);
		mSettings = &settings;
		open(ui);
	}

	void CanvasAppearanceDialog::apply() {
		if (!mSettings) return;
		mSettings->boardColor = { boardHue.value(), boardSaturation.value(), boardValue.value() };
		mSettings->canvasShadow = shadow.isChecked();
		mSettings->viewStiffness = viewStiffness.value();
	}

	Sketch2DRoot::Sketch2DRoot(
		Document& document,
		ViewportRenderer& renderer,
		Brush& brush,
		const IconProvider& icons,
		std::string sketchesRoot
	) :
		Panel(Surface::Window),
		toolBar(document, brush, icons),
		toolStrip(brush, icons),
		canvas(document, renderer, brush),
		adjustments(document),
		historyLimit(document),
		sketches(std::move(sketchesRoot)),
		library(sketches, document),
		mDocument(document),
		mRenderer(renderer),
		mBrush(brush) {
		setDebugName("sketch2d");
		setSizing(AxisSizing::fill(), AxisSizing::fill());
		emplaceLayout<ColumnLayout>();

		defineCommands();
		defineBrushCommands();
		defineColorCommands();
		buildMenus();

		canvas.onRefused = [this](const std::string& message) { notify(message); };
		canvas.heldAdjustment = [this]() { return heldBrushAdjustment(); };
		canvas.heldPicker = [this]() { return ui() && ui()->isKeyDown(InputID::LEFT_ALT); };
		canvas.onColorPicked = [this](const RGBA& color) {
			const HueColor picked = hueColorOf(color);
			const bool gray = picked.saturation <= 1e-6f || picked.value <= 1e-6f;
			colors.setPrimary(
				HueColor{ gray ? colors.primary().hueDegrees : picked.hueDegrees, picked.saturation, picked.value }
			);
			mBrush.color = colors.primary().rgba();
			if (canvas.settings().picker.sampleOpacity) mBrush.set(BrushParameter::Opacity, std::max(0.01f, color.a));
			ensureNotErasing(mBrush);
			toolBar.colorPopup.setPrimary(colors.primary());
			toolBar.refreshFromState();
		};
		toolStrip.setCanvasTool = [this](CanvasTool tool) {
			canvas.setTool(tool);
			toolStrip.refreshFromState();
		};
		toolStrip.canvasTool = [this]() { return canvas.tool(); };
		canvas.gradientEndColor = [this]() { return colors.secondary().rgba(); };
		canvas.onTransformRefused = [this]() { notify("You can't transform an empty layer"); };
		canvas.onSelectionChanged = [this]() { refreshFromState(); };
		canvas.onWand = [this](BucketResult result) {
			switch (result) {
				case BucketResult::Filled:
				case BucketResult::Leak:
				case BucketResult::Locked: break;
				case BucketResult::Empty: notify("You can't magic wand an empty area"); break;
				case BucketResult::TooBig: notify("Area too big for magic wand tool"); break;
			}
		};
		canvas.onCrop = [this](CropResult result) {
			switch (result) {
				case CropResult::Cropped:
				case CropResult::Unchanged: refreshFromState(); break;
				case CropResult::NoRegion: notify("Please choose a region to crop"); break;
				case CropResult::AlphaLocked: notify("You can't crop a alpha locked layer"); break;
			}
		};
		canvas.onTransformActive = [this](bool active) {
			transformBar.setVisible(active);
			toolStrip.refreshFromState();
		};
		canvas.onTransformApplied = [this](bool) { refreshFromState(); };
		transformBar.apply.setOnActivate([this]() {
			canvas.applyTransform();
			refreshFromState();
		});
		transformBar.cancel.setOnActivate([this]() {
			canvas.cancelTransform();
			refreshFromState();
		});
		transformBar.flip.setOnActivate([this]() { canvas.flipTransform(true); });
		transformBar.flop.setOnActivate([this]() { canvas.flipTransform(false); });
		transformBar.proportional.setOnActivate([this]() {
			canvas.settings().transform.proportional = !canvas.settings().transform.proportional;
			transformBar.proportional.setSelected(canvas.settings().transform.proportional);
		});
		canvas.onMoveFinished = [this](bool moved) {
			if (moved) refreshFromState();
			else if (!mDocument.activeLayer() || mDocument.activeLayer()->tileCount() == 0)
				notify("Nothing to move on this layer");
			else if (!mDocument.canEdit(*mDocument.activeLayer())) notify("The layer is locked");
		};
		canvas.onGradient = [this](GradientResult result) {
			switch (result) {
				case GradientResult::Drawn: refreshFromState(); break;
				case GradientResult::NoFrame: notify("You must have a framed canvas or a selection to use gradient"); break;
				case GradientResult::Locked: notify("The layer is locked"); break;
			}
		};
		canvas.onBucket = [this](BucketResult result) {
			switch (result) {
				case BucketResult::Filled: refreshFromState(); break;
				case BucketResult::Leak: notify("Found a leak. Please choose an enclosed area or lower threshold"); break;
				case BucketResult::Empty: notify("You can't paint bucket an empty area"); break;
				case BucketResult::TooBig: notify("Area too big for paint bucket tool"); break;
				case BucketResult::Locked: notify("The layer is locked"); break;
			}
		};
		canvas.onBrushAdjusted = [this]() { toolBar.refreshFromState(); };
		toolBar.brushPopup.onReset = [this]() { brushChanged("Reset brush"); };
		toolBar.colorPopup.onPicked = [this](const HueColor& color) {
			colors.setPrimary(color);
			toolBar.refreshFromState();
			canvas.requestPaint();
		};
		toolBar.colorPopup.onCopy = [this]() { commands.run(Command::CopyColor); };
		toolBar.colorPopup.onAddSwatch = [this]() { commands.run(Command::AddSwatch); };
		toolBar.colorPopup.onRemoveSwatch = [this](size_t index) {
			swatches.remove(index);
			if (!swatchPath.empty()) swatches.save(swatchPath);
			toolBar.colorPopup.showSwatches(swatches);
			notify("Removed swatch");
		};
		canvas.onStrokeFinished = [this]() {
			if (mBrush.erases() || mBrush.smudges()) return;
			pullBrushColor();
			colorHistory.recordStroke(colors.primary());
			toolBar.colorPopup.showHistory(colorHistory);
		};
		swatchPath = (std::filesystem::path(sketches.root()) / "swatches.json").string();
		swatches = SwatchSet::load(swatchPath);
		toolBar.colorPopup.showSwatches(swatches);
		toolBar.color.setOnOpening([this]() {
			pullBrushColor();
			toolBar.colorPopup.setPrimary(colors.primary());
			toolBar.colorPopup.markPrevious();
			toolBar.colorPopup.refresh();
		});
		library.onNotice = [this](const std::string& message) { notify(message); };
		library.onDocumentReplaced = [this]() {
			toolBar.layersPopup.refresh();
			refreshFromState();
		};
		toolBar.layersPopup.onChanged = [this]() { refreshFromState(); };
		toolBar.layersPopup.onRename = [this](Layer& layer) {
			mDocument.setActiveLayer(&layer);
			commands.run(Command::RenameLayer);
		};
		toolBar.undo.setOnActivate([this]() {
			commands.run(Command::Undo);
			refreshFromState();
		});
		toolBar.redo.setOnActivate([this]() {
			commands.run(Command::Redo);
			refreshFromState();
		});

		mBody.setSizing(AxisSizing::fill(), AxisSizing::fill());
		mBody.emplaceLayout<RowLayout>();
		mBody.addChild(toolStrip);
		mBody.addChild(canvas);
		transformBar.setVisible(false);

		addChild(menuBar);
		addChild(toolBar);
		addChild(transformBar);
		addChild(mBody);
		refreshFromState();
	}

	void Sketch2DRoot::refreshFromState() {
		if (mDocument.history().revision() != mShownRevision && !toolBar.layersPopup.list().isDragging()) {
			mShownRevision = mDocument.history().revision();
			toolBar.layersPopup.refresh();
		}
		toolBar.undo.setEnabled(commands.isEnabled(Command::Undo));
		toolBar.redo.setEnabled(commands.isEnabled(Command::Redo));
		toolBar.undo.setTooltip(commands.isEnabled(Command::Undo) ? "Undo: " + mDocument.history().undoLabel() : "Undo");
		toolBar.redo.setTooltip(commands.isEnabled(Command::Redo) ? "Redo: " + mDocument.history().redoLabel() : "Redo");
		toolBar.refreshFromState();
		toolStrip.refreshFromState();
		if (onStateRefreshed) onStateRefreshed();
	}

	void Sketch2DRoot::onAttached() {
		canvas.onFingerTap = [this](int4 fingers) {
			if (fingers == 2) commands.run(Command::Undo);
			else if (fingers == 3) commands.run(Command::Redo);
			refreshFromState();
		};
		ui()->setOnEventDispatched([this]() {
			refreshFromState();
			if (library.autosaves() && library.hasUnsavedChanges()) startTicking();
		});
		ui()->setOnAdvance([this](halnf deltaMs) { commands.advanceClock(deltaMs); });
	}

	void Sketch2DRoot::onEvent(Event& event) {
		if (event.type() == Event::Type::KeyDown && canvas.isTransforming()) {
			if (event.keyCode() == InputID::ENTER || event.keyCode() == InputID::KP_ENTER) {
				canvas.applyTransform();
				refreshFromState();
				event.accept();
				return;
			}
			if (event.keyCode() == InputID::ESCAPE) {
				canvas.cancelTransform();
				refreshFromState();
				event.accept();
				return;
			}
		}
		if (event.type() == Event::Type::KeyDown && !canvas.polygonPoints().empty()) {
			if (event.keyCode() == InputID::ENTER || event.keyCode() == InputID::KP_ENTER) {
				canvas.finishPolygon();
				event.accept();
				return;
			}
			if (event.keyCode() == InputID::ESCAPE) {
				canvas.cancelPolygon();
				event.accept();
				return;
			}
		}
		if (
			event.type() == Event::Type::KeyDown && event.keyCode() == InputID::ESCAPE && mDocument.hasSelection() &&
			!event.modifiers().control && !event.modifiers().shift && !event.modifiers().alt
		) {
			commands.run(Command::Deselect);
			event.accept();
			return;
		}
		if (event.type() == Event::Type::KeyDown && commands.handleKey(event)) refreshFromState();
	}

	MenuItem& Sketch2DRoot::addCommand(Menu& menu, Command command, std::function<std::string()> dynamicText) {
		MenuEntry entry = MenuEntry::item(
			commandInfo(command).label,
			[this, command]() {
				commands.run(command);
				refreshFromState();
			},
			commands.shortcutText(command)
		);
		entry.isEnabled = [this, command]() { return commands.isEnabled(command); };
		entry.isChecked = [this, command]() { return commands.isChecked(command); };
		entry.dynamicText = std::move(dynamicText);
		return menu.add(std::move(entry));
	}

	void Sketch2DRoot::layerChanged(Command command, bool changed, const std::string& mergeKey) {
		if (!changed) return;
		mDocument.commit(commandInfo(command).history, mergeKey);
		toolBar.layersPopup.refresh();
	}

	void Sketch2DRoot::defineLayerPropertyCommands() {
		Document& document = mDocument;
		auto hasLayer = [&document]() { return document.activeLayer() != nullptr; };

		commands.define(
			Command::ToggleLayerVisible,
			[this, &document](const Event*) { layerChanged(Command::ToggleLayerVisible, document.toggleVisible()); },
			hasLayer,
			[&document]() { return document.activeLayer() && document.activeLayer()->visible(); }
		);
		commands.define(
			Command::ToggleLayerClip,
			[this, &document](const Event*) { layerChanged(Command::ToggleLayerClip, document.toggleClip(), "layer_clip"); },
			[&document]() { return document.activeLayer() && document.layerBelow(*document.activeLayer()) != nullptr; },
			[&document]() { return document.activeLayer() && document.activeLayer()->clipped(); }
		);
		commands.define(
			Command::ToggleLayerLock,
			[this, &document](const Event*) { layerChanged(Command::ToggleLayerLock, document.toggleLock(), "layer_lock"); },
			hasLayer,
			[&document]() { return document.activeLayer() && document.activeLayer()->locked(); }
		);
		commands.define(
			Command::ToggleLayerAlphaLock,
			[this, &document](const Event*) {
				layerChanged(Command::ToggleLayerAlphaLock, document.toggleAlphaLock(), "layer_lock_alpha");
			},
			hasLayer,
			[&document]() { return document.activeLayer() && document.activeLayer()->alphaLocked(); }
		);

		commands.define(
			Command::PreviousLayerBlend,
			[this, &document](const Event*) {
				layerChanged(Command::PreviousLayerBlend, document.cycleBlend(-1), "layer_blend");
			},
			hasLayer
		);
		commands.define(
			Command::NextLayerBlend,
			[this, &document](const Event*) { layerChanged(Command::NextLayerBlend, document.cycleBlend(1), "layer_blend"); },
			hasLayer
		);
		commands.define(
			Command::NormalLayerBlend,
			[this, &document](const Event*) {
				layerChanged(Command::NormalLayerBlend, document.resetBlend(), "layer_blend");
			},
			hasLayer,
			[&document]() { return document.activeLayer() && document.activeLayer()->blend() == BlendMode::Normal; }
		);

		commands.define(
			Command::LayerOpacityDecrease,
			[this, &document](const Event*) {
				layerChanged(Command::LayerOpacityDecrease, document.changeOpacity(-0.1f), "layer_opacity");
			},
			hasLayer
		);
		commands.define(
			Command::LayerOpacityIncrease,
			[this, &document](const Event*) {
				layerChanged(Command::LayerOpacityIncrease, document.changeOpacity(0.1f), "layer_opacity");
			},
			hasLayer
		);

		for (int4 tenth = 0; tenth <= 10; tenth++) {
			const auto command = (Command) ((int4) Command::LayerOpacity0 + tenth);
			commands.define(
				command,
				[this, &document, command, tenth](const Event*) {
					const bool fine = tenth < 10 && commands.isRepeatOf(command);
					const halnf opacity = fine ? (halnf) tenth / 100.0f : (halnf) tenth / 10.0f;
					layerChanged(command, document.setOpacity(opacity), "layer_opacity");
				},
				hasLayer,
				[&document, tenth]() {
					return document.activeLayer() &&
								 std::fabs(document.activeLayer()->opacity() - (halnf) tenth / 10.0f) < 0.001f;
				}
			);
		}
	}

	void Sketch2DRoot::notify(const std::string& message) {
		mLastNotice = message;
		notice.show(canvas, message);
	}

	void Sketch2DRoot::defineLayerStructureCommands() {
		Document& document = mDocument;
		auto hasLayer = [&document]() { return document.activeLayer() != nullptr; };

		commands.define(
			Command::NewLayer,
			[this, &document](const Event*) {
				if (!document.addLayer(document.nextLayerName())) return;
				layerChanged(Command::NewLayer, true);
			},
			[&document]() { return document.canAddLayer(); }
		);
		commands.define(
			Command::DuplicateLayer,
			[this, &document](const Event*) {
				if (!document.activeLayer() || !document.duplicateLayer(*document.activeLayer())) return;
				layerChanged(Command::DuplicateLayer, true);
			},
			[&document]() { return document.activeLayer() && document.canAddLayer(); }
		);
		commands.define(
			Command::DeleteLayer,
			[this, &document](const Event*) {
				if (document.activeLayer()->locked()) {
					notify("You can't delete a locked layer");
					return;
				}
				document.removeActiveLayer();
				layerChanged(Command::DeleteLayer, true);
			},
			[&document]() { return document.layerCount() > 1; }
		);
		commands.define(
			Command::MoveLayerUp,
			[this, &document](const Event*) { layerChanged(Command::MoveLayerUp, document.moveActiveLayer(1)); },
			[&document]() {
				return document.activeLayer() && document.indexOf(*document.activeLayer()) + 1 < document.layerCount();
			}
		);
		commands.define(
			Command::MoveLayerDown,
			[this, &document](const Event*) { layerChanged(Command::MoveLayerDown, document.moveActiveLayer(-1)); },
			[&document]() { return document.activeLayer() && document.indexOf(*document.activeLayer()) > 0; }
		);
		commands.define(
			Command::RenameLayer,
			[this, &document](const Event*) {
				if (!ui() || !document.activeLayer()) return;
				Layer* layer = document.activeLayer();
				renameLayer.show(*ui(), layer->name(), [this, &document, layer](const std::string& name) {
					layerChanged(Command::RenameLayer, document.renameLayer(*layer, name));
					refreshFromState();
				});
			},
			hasLayer
		);
	}

	void Sketch2DRoot::defineLayerSelectionCommands() {
		Document& document = mDocument;
		auto multipleLayers = [&document]() { return document.layerCount() > 1; };

		commands.define(
			Command::SelectLayerAbove,
			[this, &document](const Event*) {
				layerChanged(Command::SelectLayerAbove, document.selectAdjacent(1), "layer_select");
			},
			multipleLayers
		);
		commands.define(
			Command::SelectLayerBelow,
			[this, &document](const Event*) {
				layerChanged(Command::SelectLayerBelow, document.selectAdjacent(-1), "layer_select");
			},
			multipleLayers
		);
		commands.define(
			Command::SelectAllLayers,
			[this, &document](const Event*) { layerChanged(Command::SelectAllLayers, document.selectAllLayers()); },
			multipleLayers
		);
	}

	void Sketch2DRoot::defineLayerMergeCommands() {
		Document& document = mDocument;

		commands.define(
			Command::MergeLayers,
			[this, &document](const Event*) {
				layerChanged(Command::MergeLayers, document.mergeLayers(document.mergeCandidates()));
			},
			[&document]() { return document.mergeCandidates().size() >= 2; }
		);
		commands.define(
			Command::MergeAllLayers,
			[this, &document](const Event*) {
				if (!ui()) return;
				confirm.ask(
					*ui(),
					"Merge all layers",
					"Are you sure you would like to merge all layers?",
					"Merge all",
					[this, &document]() {
						layerChanged(Command::MergeAllLayers, document.mergeVisibleLayers());
						refreshFromState();
					}
				);
			},
			[&document]() { return document.layerCount() > 1; }
		);
		commands.define(
			Command::BakeLayerOpacity,
			[this, &document](const Event*) {
				bool changed = false;
				for (Layer* layer : document.targetLayers()) {
					if (document.canEdit(*layer)) changed |= document.bakeOpacity(*layer);
				}
				layerChanged(Command::BakeLayerOpacity, changed);
			},
			[&document]() {
				const auto layers = document.targetLayers();
				return std::any_of(layers.begin(), layers.end(), [](const Layer* layer) { return layer->opacity() < 1.0f; });
			}
		);
	}

	void Sketch2DRoot::defineCanvasCommands() {
		Document& document = mDocument;

		commands.define(Command::CanvasBackground, [this, &document](const Event*) {
			if (!ui()) return;
			canvasBackground.show(
				*ui(),
				document.background(),
				[this, &document](const CanvasBackground& background) {
					document.previewBackground(background);
					canvas.requestPaint();
				},
				[this, &document](const CanvasBackground& before, const CanvasBackground& background) {
					document.previewBackground(before);
					if (document.setBackground(background)) document.commit("Background");
					canvas.requestPaint();
					refreshFromState();
				}
			);
		});
		commands.define(
			Command::InfiniteCanvas,
			[this, &document](const Event*) {
				const Vec2F center = document.screenToWorld(canvas.size() / 2.0f);
				if (document.setInfinite(!document.isInfinite(), center)) document.commit("Infinite canvas");
			},
			{},
			[&document]() { return document.isInfinite(); }
		);
		commands.define(Command::CanvasFrameSize, [this, &document](const Event*) {
			if (!ui()) return;
			const CanvasFrame proposed = document.proposedFrame(document.screenToWorld(canvas.size() / 2.0f));
			canvasFrameSize.show(*ui(), proposed, [this, &document, proposed](int4 width, int4 height) {
				const int4 centerX = (proposed.left + proposed.right) / 2;
				const int4 centerY = (proposed.top + proposed.bottom) / 2;
				bool changed = document.setInfinite(false);
				changed |= document.setFrame(
					centerX - width / 2, centerY - height / 2, centerX - width / 2 + width, centerY - height / 2 + height
				);
				if (changed) document.commit("Canvas frame size");
				refreshFromState();
			});
		});
		commands.define(
			Command::ShowChannel,
			[this](const Event*) {
				canvas.settings().showChannel = !canvas.settings().showChannel;
				canvas.requestPaint();
			},
			{},
			[this]() { return canvas.settings().showChannel; }
		);
		commands.define(Command::CanvasAppearance, [this](const Event*) {
			if (ui()) canvasAppearance.show(*ui(), canvas.settings());
		});
	}

	void Sketch2DRoot::defineCommands() {
		Document& document = mDocument;
		defineSessionCommands();
		defineLayerPropertyCommands();
		defineLayerStructureCommands();
		defineLayerSelectionCommands();
		defineLayerMergeCommands();
		defineCanvasCommands();

		commands.define(Command::StabilizerDecrease, [this](const Event*) {
			setStabilizer(canvas.activeStabilizer() - 0.1f);
		});
		commands.define(Command::StabilizerIncrease, [this](const Event*) {
			setStabilizer(canvas.activeStabilizer() + 0.1f);
		});
		commands.define(Command::NewInfiniteCanvas, [this, &document](const Event*) {
			library.saveIfChanged();
			document.reset();
			library.setCurrentName({});
			library.markSaved();
			toolBar.layersPopup.refresh();
		});
		commands.define(Command::Open, [this](const Event*) {
			if (ui()) library.open(*ui());
		});
		commands.define(Command::Save, [this](const Event*) { library.saveCurrent(); });
		commands.define(Command::SaveAs, [this](const Event*) {
			if (!ui()) return;
			const std::string suggestion = library.currentName().empty() ? sketches.unusedName() : library.currentName();
			library.saveDialog.show(*ui(), suggestion, [this](const std::string& name) { library.saveCurrentAs(name); });
		});
		commands.define(Command::ZoomIn, [this](const Event*) {
			canvas.changeView([](Document& view) { view.setZoom(view.zoom() * 1.25f); });
		});
		commands.define(Command::ZoomOut, [this](const Event*) {
			canvas.changeView([](Document& view) { view.setZoom(view.zoom() / 1.25f); });
		});
		commands.define(Command::ResetZoom, [this](const Event*) {
			canvas.changeView([](Document& view) { view.setZoom(1.0f); });
		});
		commands.define(Command::FitOnScreen, [this](const Event*) {
			canvas.changeView([](Document& view) { view.resetView(); });
		});
		const auto rotateView = [this](halnf radians) {
			const Vec2F center = canvas.size() / 2.0f;
			canvas.changeView([center, radians](Document& view) {
				if (radians == 0) view.setRotationAt(center, 0);
				else view.rotateAt(center, radians);
			});
		};
		commands.define(Command::RotateLeft, [rotateView](const Event*) { rotateView(-kRotationStepRadians); });
		commands.define(Command::RotateRight, [rotateView](const Event*) { rotateView(kRotationStepRadians); });
		commands.define(Command::ResetRotation, [rotateView](const Event*) { rotateView(0); });
		commands.define(Command::ClearCanvas, [&document](const Event*) {
			document.clearActiveLayer();
			document.commit(commandInfo(Command::ClearCanvas).history);
		});
		commands.define(Command::ClearLayer, [this, &document](const Event*) {
			bool changed = false;
			document.settlePaint();
			for (Layer* layer : document.targetLayers()) {
				if (!document.canEdit(*layer)) {
					notify("You can't edit a locked layer");
					continue;
				}
				if (layer->alphaLocked()) {
					notify("You can't clear a alpha locked layer");
					continue;
				}
				changed |= layer->tileCount() > 0;
				layer->clear();
			}
			layerChanged(Command::ClearLayer, changed);
		});

		commands.define(
			Command::Undo,
			[this, &document](const Event*) {
				document.undo();
				toolBar.layersPopup.refresh();
			},
			[&document]() { return document.history().canUndo(); }
		);
		commands.define(
			Command::Redo,
			[this, &document](const Event*) {
				document.redo();
				toolBar.layersPopup.refresh();
			},
			[&document]() { return document.history().canRedo(); }
		);
		commands.define(
			Command::ClearHistory,
			[this, &document](const Event*) {
				if (!ui()) return;
				confirm.ask(
					*ui(), "Clear history", "Are you sure you want to clear history?", "Clear history", [this, &document]() {
						document.history().clear();
						refreshFromState();
					}
				);
			},
			[&document]() { return document.history().entries().size() > 1; }
		);
		commands.define(Command::SetHistoryLimit, [this](const Event*) {
			if (ui()) historyLimit.show(*ui());
		});
		commands.define(Command::ToolPaint, [this](const Event*) { toolStrip.button("brush").activate(); });
		commands.define(Command::ToolEraser, [this](const Event*) { toolStrip.button("eraser").activate(); });
		commands.define(Command::ToolSmudge, [this](const Event*) { toolStrip.button("smudge").activate(); });
		commands.define(Command::ToolPicker, [this](const Event*) { toolStrip.button("eyedropper").activate(); });
		commands.define(Command::ToolHand, [this](const Event*) { toolStrip.button("hand").activate(); });
		commands.define(Command::ToolBucket, [this](const Event*) { toolStrip.button("fill").activate(); });
		commands.define(Command::ToolGradient, [this](const Event*) { toolStrip.button("gradient").activate(); });
		commands.define(Command::ToolMove, [this](const Event*) { toolStrip.button("move").activate(); });
		commands.define(Command::FreeTransform, [this](const Event*) {
			if (canvas.isTransforming()) canvas.applyTransform();
			else toolStrip.button("transform").activate();
		});
		commands.define(Command::Transform, [this](const Event*) { commands.run(Command::FreeTransform); });
		commands.define(Command::ToolRectangleSelect, [this](const Event*) { toolStrip.button("rectselect").activate(); });
		commands.define(Command::ToolLasso, [this](const Event*) { toolStrip.button("lasso").activate(); });
		commands.define(Command::ToolMagicWand, [this](const Event*) { toolStrip.button("wand").activate(); });
		const auto selectionChanged = [this, &document](bool changed, Command command) {
			if (changed) document.commit(commandInfo(command).history);
			canvas.requestPaint();
			refreshFromState();
		};
		const auto selectionEdit = [this, &document](Document::SelectionEditResult result, Command command) {
			switch (result) {
				case Document::SelectionEditResult::Done: document.commit(commandInfo(command).history); break;
				case Document::SelectionEditResult::NoSelection: notify("Please select an area before you fill"); break;
				case Document::SelectionEditResult::Inverted: notify("You can't fill the invert of a selection"); break;
				case Document::SelectionEditResult::AlphaLocked: notify("You can't clear a alpha locked layer"); break;
				case Document::SelectionEditResult::Locked: notify("You can't edit a locked layer"); break;
			}
			canvas.requestPaint();
			refreshFromState();
		};
		commands.define(Command::FillSelection, [this, &document, selectionEdit](const Event*) {
			selectionEdit(document.fillSelection(colors.primary().rgba()), Command::FillSelection);
		});
		commands.define(Command::FillUnderneath, [this, &document, selectionEdit](const Event*) {
			selectionEdit(document.fillSelectionUnderneath(colors.primary().rgba()), Command::FillUnderneath);
		});
		commands.define(Command::Clear, [&document, selectionEdit](const Event*) {
			selectionEdit(document.clearSelection(), Command::Clear);
		});
		const auto filterCommand = [this, &document](Command command, Document::PixelFilter filter) {
			commands.define(command, [this, &document, command, filter](const Event*) {
				switch (document.applyFilter(filter)) {
					case Document::FilterResult::Done: document.commit(commandInfo(command).history); break;
					case Document::FilterResult::Empty: notify("The layer is empty"); break;
					case Document::FilterResult::Locked: notify("You can't edit a locked layer"); break;
					case Document::FilterResult::AlphaLocked: notify("You can't change alpha on a alpha locked layer"); break;
				}
				canvas.requestPaint();
				refreshFromState();
			});
		};
		filterCommand(Command::Invert, Document::PixelFilter::Invert);
		filterCommand(Command::Desaturate, Document::PixelFilter::Desaturate);
		filterCommand(Command::ValueToAlpha, Document::PixelFilter::ValueToAlpha);
		filterCommand(Command::ValueToAlphaInverted, Document::PixelFilter::ValueToAlphaInverted);
		commands.define(Command::Copy, [this, &document](const Event*) {
			Document::PixelClip clip = document.copyPixels();
			if (clip.isEmpty()) {
				notify("Nothing to copy");
				return;
			}
			clipboard = std::move(clip);
			notify("Copied");
		});
		commands.define(Command::Cut, [this, &document, selectionEdit](const Event*) {
			Document::PixelClip clip = document.copyPixels();
			if (clip.isEmpty()) {
				notify("Nothing to cut");
				return;
			}
			clipboard = std::move(clip);
			selectionEdit(document.clearSelection(), Command::Cut);
		});
		commands.define(Command::Paste, [this, &document](const Event*) {
			if (clipboard.isEmpty()) {
				notify("The clipboard is empty");
				return;
			}
			if (!document.pasteAsLayer(clipboard, "Pasted")) {
				notify("You can't add more layers");
				return;
			}
			document.deselect();
			document.commit(commandInfo(Command::Paste).history);
			refreshFromState();
			canvas.setTool(CanvasTool::Transform);
			toolStrip.refreshFromState();
		});
		commands.define(Command::CopyToNewLayer, [this, &document](const Event*) {
			const Document::PixelClip clip = document.copyPixels();
			if (clip.isEmpty()) {
				notify("Nothing to copy");
				return;
			}
			if (!document.pasteAsLayer(clip, "Copy")) {
				notify("You can't add more layers");
				return;
			}
			document.commit(commandInfo(Command::CopyToNewLayer).history);
			refreshFromState();
		});
		const auto adjustCommand = [this, &document](Command command, AdjustmentKind kind) {
			commands.define(command, [this, &document, command, kind](const Event*) {
				if (!ui()) return;
				if (!document.beginAdjustment()) {
					notify(
						document.activeLayer() && document.activeLayer()->tileCount() == 0 ? "The layer is empty"
																																							 : "You can't edit a locked layer"
					);
					return;
				}
				adjustments.show(*ui(), kind, [this, &document, command](bool changed) {
					if (changed) document.commit(commandInfo(command).history);
					canvas.requestPaint();
					refreshFromState();
				});
			});
		};
		adjustCommand(Command::AdjustValue, AdjustmentKind::BrightnessExposure);
		adjustCommand(Command::AdjustHueSaturation, AdjustmentKind::HueSaturation);
		adjustCommand(Command::AdjustColorBalance, AdjustmentKind::ColorBalance);
		adjustCommand(Command::AdjustColorize, AdjustmentKind::Colorize);
		adjustCommand(Command::AdjustAlpha, AdjustmentKind::Alpha);
		commands.define(Command::SpreadSelection, [this, &document, selectionChanged](const Event*) {
			if (!document.hasSelection()) return;
			document.editSelection().spread(canvas.settings().selection.adjustPixels);
			selectionChanged(true, Command::SpreadSelection);
		});
		commands.define(Command::BlurSelection, [this, &document, selectionChanged](const Event*) {
			if (!document.hasSelection()) return;
			document.editSelection().blur(canvas.settings().selection.adjustPixels);
			selectionChanged(true, Command::BlurSelection);
		});
		commands.define(Command::Deselect, [&document, selectionChanged](const Event*) {
			selectionChanged(document.deselect(), Command::Deselect);
		});
		commands.define(Command::InvertSelection, [&document, selectionChanged](const Event*) {
			selectionChanged(document.invertSelection(), Command::InvertSelection);
		});
		commands.define(Command::SelectAllAsLasso, [this, &document, selectionChanged](const Event*) {
			const bool changed = document.selectAllAsLasso();
			if (!changed) notify("Nothing to select");
			selectionChanged(changed, Command::SelectAllAsLasso);
		});
		commands.define(Command::SelectAllAsRectangle, [this, &document, selectionChanged](const Event*) {
			const bool changed = document.selectAllAsRectangle();
			if (!changed) notify("Nothing to select");
			selectionChanged(changed, Command::SelectAllAsRectangle);
		});
		commands.define(Command::SelectCanvasFrame, [this, &document, selectionChanged](const Event*) {
			if (document.isInfinite()) {
				notify("The canvas has no frame");
				return;
			}
			selectionChanged(document.selectAllCanvas(), Command::SelectCanvasFrame);
		});
		commands.define(Command::Crop, [this, &document](const Event*) {
			if (document.isInfinite()) {
				toolStrip.button("crop").activate();
				notify("Please choose a region to crop");
				return;
			}
			const CanvasFrame& frame = document.frame();
			const CropResult result = document.cropTo(frame.left, frame.top, frame.right, frame.bottom);
			if (result == CropResult::Cropped) document.commit(commandInfo(Command::Crop).history);
			canvas.onCrop(result);
		});
		commands.define(Command::FlipHorizontal, [this, &document](const Event*) {
			if (canvas.isTransforming()) {
				canvas.flipTransform(true);
				return;
			}
			if (document.flipActiveLayer(true)) document.commit(commandInfo(Command::FlipHorizontal).history);
			else notify("You can't transform an empty layer");
		});
		commands.define(Command::FlipVertical, [this, &document](const Event*) {
			if (canvas.isTransforming()) {
				canvas.flipTransform(false);
				return;
			}
			if (document.flipActiveLayer(false)) document.commit(commandInfo(Command::FlipVertical).history);
			else notify("You can't transform an empty layer");
		});
		commands.define(Command::ToolZoom, [this](const Event*) { toolStrip.button("zoom").activate(); });
		commands.define(Command::ToolRotate, [this](const Event*) { toolStrip.button("rotate").activate(); });
	}

	void Sketch2DRoot::buildHistoryMenu(Menu& menu) {
		constexpr size_t kShownRedoStates = 6;
		constexpr size_t kShownUndoStates = 12;

		menu.clear();
		const auto& history = mDocument.history();
		const auto& entries = history.entries();
		const size_t current = history.currentIndex();

		const size_t newestShown = std::min(entries.size() - 1, current + kShownRedoStates);
		if (newestShown + 1 < entries.size()) {
			MenuItem& more = menu.addItem("(" + std::to_string(entries.size() - 1 - newestShown) + " More)", {});
			more.setEnabled(false);
		}

		const size_t oldestShown = current > kShownUndoStates ? current - kShownUndoStates : 0;
		for (size_t index = newestShown + 1; index-- > oldestShown;) {
			MenuEntry entry = MenuEntry::item(entries[index].label, [this, index]() {
				mDocument.jumpToHistory(index);
				toolBar.layersPopup.refresh();
				refreshFromState();
			});
			entry.isChecked = [this, index]() { return mDocument.history().currentIndex() == index; };
			entry.muted = index > current;
			menu.add(std::move(entry));
		}

		if (oldestShown > 0) {
			MenuItem& more = menu.addItem("(" + std::to_string(oldestShown) + " More)", {});
			more.setEnabled(false);
		}

		menu.addSeparator();
		addCommand(menu, Command::ClearHistory);
		addCommand(menu, Command::SetHistoryLimit);
	}

	void Sketch2DRoot::buildMenus() {
		ViewportRenderer& renderer = mRenderer;
		Document& document = mDocument;

		addCommand(fileMenu, Command::NewInfiniteCanvas);
		addCommand(fileMenu, Command::Open);
		addCommand(fileMenu, Command::Save);
		addCommand(fileMenu, Command::SaveAs);
		fileMenu.addToggle(
			"Autosave", [this]() { return library.autosaves(); }, [this]() { library.setAutosave(!library.autosaves()); }
		);
		fileMenu.addSeparator();
		addCommand(fileMenu, Command::Import);
		addCommand(fileMenu, Command::Export);

		addCommand(editMenu, Command::Undo, [&document]() {
			const auto& history = document.history();
			return history.canUndo() ? "Undo: " + history.undoLabel() : std::string("Undo");
		});
		addCommand(editMenu, Command::Redo, [&document]() {
			const auto& history = document.history();
			return history.canRedo() ? "Redo: " + history.redoLabel() : std::string("Redo");
		});
		editMenu.addSeparator();
		addCommand(editMenu, Command::Cut);
		addCommand(editMenu, Command::Copy);
		addCommand(editMenu, Command::Paste);
		addCommand(editMenu, Command::CopyToNewLayer);
		editMenu.addSeparator();
		addCommand(editMenu, Command::FillSelection);
		addCommand(editMenu, Command::Clear);
		addCommand(editMenu, Command::Crop);
		addCommand(editMenu, Command::FreeTransform);
		addCommand(editMenu, Command::FlipHorizontal);
		addCommand(editMenu, Command::FlipVertical);
		editMenu.addSeparator();
		addCommand(editMenu, Command::Invert);
		addCommand(editMenu, Command::Desaturate);
		addCommand(editMenu, Command::ValueToAlpha);
		addCommand(editMenu, Command::ValueToAlphaInverted);
		addCommand(adjustImageMenu, Command::AdjustValue);
		addCommand(adjustImageMenu, Command::AdjustHueSaturation);
		addCommand(adjustImageMenu, Command::AdjustColorBalance);
		addCommand(adjustImageMenu, Command::AdjustColorize);
		addCommand(adjustImageMenu, Command::AdjustAlpha);
		editMenu.addSubmenu("Adjust", adjustImageMenu);

		addCommand(viewMenu, Command::ZoomIn);
		addCommand(viewMenu, Command::ZoomOut);
		addCommand(viewMenu, Command::ResetZoom);
		addCommand(viewMenu, Command::FitOnScreen);
		viewMenu.addSeparator();
		addCommand(viewMenu, Command::RotateLeft);
		addCommand(viewMenu, Command::RotateRight);
		addCommand(viewMenu, Command::ResetRotation);
		commands.define(
			Command::LockView,
			[this](const Event*) {
				canvas.settings().lockView = !canvas.settings().lockView;
				notify(canvas.settings().lockView ? "View locked" : "View unlocked");
			},
			{},
			[this]() { return canvas.settings().lockView; }
		);
		commands.define(
			Command::FullScreen,
			[this](const Event*) {
				if (setFullScreen) setFullScreen(!(isFullScreen && isFullScreen()));
			},
			[this]() { return (bool) setFullScreen; },
			[this]() { return isFullScreen && isFullScreen(); }
		);
		addCommand(viewMenu, Command::LockView);
		addCommand(viewMenu, Command::FullScreen);
		viewMenu.addSeparator();
		addCommand(viewMenu, Command::ShowChannel);
		viewMenu.addSubmenu("Channel", channelMenu);
		viewMenu.addSubmenu("Cursor", cursorMenu);

		for (int4 index = 0; index < kViewChannelCount; index++) {
			const ViewChannel channel = (ViewChannel) index;
			channelMenu.addToggle(
				viewChannelName(channel),
				[this, channel]() { return canvas.settings().showChannel && canvas.settings().channel == channel; },
				[this, channel]() {
					canvas.settings().showChannel = true;
					canvas.settings().channel = channel;
					canvas.requestPaint();
				}
			);
		}

		buildCursorMenu();

		addCommand(canvasMenu, Command::CanvasFrameSize);
		addCommand(canvasMenu, Command::CanvasBackground);
		addCommand(canvasMenu, Command::CanvasAppearance);
		canvasMenu.addSeparator();
		addCommand(canvasMenu, Command::InfiniteCanvas);
		canvasMenu.addSeparator();
		addCommand(canvasMenu, Command::ClearCanvas);
		helpMenu.addItem("About " + publicName, [this]() { notify(publicName + " " + versionText); });

		addCommand(selectMenu, Command::SelectAllAsRectangle);
		addCommand(selectMenu, Command::SelectAllAsLasso);
		addCommand(selectMenu, Command::SelectCanvasFrame);
		addCommand(selectMenu, Command::InvertSelection);
		addCommand(selectMenu, Command::Deselect);
		selectMenu.addSeparator();
		addCommand(selectMenu, Command::SpreadSelection);
		addCommand(selectMenu, Command::BlurSelection);
		for (const int4 pixels : { -8, -4, -1, 1, 4, 8, 16 }) {
			selectAdjustMenu.add(
				MenuEntry::toggle(
					(pixels > 0 ? "+" : "") + std::to_string(pixels) + " px",
					[this, pixels]() { return canvas.settings().selection.adjustPixels == pixels; },
					[this, pixels]() { canvas.settings().selection.adjustPixels = pixels; }
				)
			);
		}
		selectMenu.addSubmenu("Spread / blur amount", selectAdjustMenu);
		selectMenu.addSeparator();
		addCommand(selectMenu, Command::FillSelection);
		addCommand(selectMenu, Command::FillUnderneath);
		addCommand(selectMenu, Command::Clear);
		selectMenu.add(
			MenuEntry::toggle(
				"Auto fill selection",
				[this]() { return canvas.settings().selection.autoFill; },
				[this]() { canvas.settings().selection.autoFill = !canvas.settings().selection.autoFill; }
			)
		);
		selectMenu.addSeparator();
		addCommand(selectMenu, Command::ToolRectangleSelect);
		addCommand(selectMenu, Command::ToolLasso);
		addCommand(selectMenu, Command::ToolMagicWand);
		selectMenu.addSeparator();
		SelectionSettings& selecting = canvas.settings().selection;
		selectMenu.add(
			MenuEntry::toggle(
				"Rectangle",
				[&selecting]() { return selecting.marquee == MarqueeShape::Rectangle; },
				[&selecting]() { selecting.marquee = MarqueeShape::Rectangle; }
			)
		);
		selectMenu.add(
			MenuEntry::toggle(
				"Ellipse",
				[&selecting]() { return selecting.marquee == MarqueeShape::Ellipse; },
				[&selecting]() { selecting.marquee = MarqueeShape::Ellipse; }
			)
		);
		selectMenu.add(
			MenuEntry::toggle(
				"Freehand lasso",
				[&selecting]() { return selecting.lasso == LassoShape::Freehand; },
				[&selecting]() { selecting.lasso = LassoShape::Freehand; }
			)
		);
		selectMenu.add(
			MenuEntry::toggle(
				"Polygon lasso",
				[&selecting]() { return selecting.lasso == LassoShape::Polygon; },
				[this, &selecting]() {
					selecting.lasso = LassoShape::Polygon;
					canvas.cancelPolygon();
				}
			)
		);
		selectMenu.add(
			MenuEntry::toggle(
				"Click to deselect",
				[&selecting]() { return selecting.clickToDeselect; },
				[&selecting]() { selecting.clickToDeselect = !selecting.clickToDeselect; }
			)
		);
		BucketSettings& wand = selecting.wand;
		wandMenu.add(
			MenuEntry::toggle(
				"Sample all visible layers",
				[&wand]() { return wand.allLayers; },
				[&wand]() { wand.allLayers = !wand.allLayers; }
			)
		);
		wandMenu.add(
			MenuEntry::toggle(
				"Sample only alpha channel",
				[&wand]() { return wand.onlyAlpha; },
				[&wand]() { wand.onlyAlpha = !wand.onlyAlpha; }
			)
		);
		wandMenu.addSeparator();
		for (const int4 percent : { 0, 5, 10, 20, 35, 50 }) {
			const halnf value = (halnf) percent / 100.0f;
			wandMenu.add(
				MenuEntry::toggle(
					"Threshold " + std::to_string(percent) + "%",
					[&wand, value]() { return std::abs(wand.threshold - value) < 1e-4f; },
					[&wand, value]() { wand.threshold = value; }
				)
			);
		}
		selectMenu.addSubmenu("Magic wand", wandMenu);
		toolBar.select.setOnActivate([this]() {
			selectMenu.openAt(toolBar.select, toolBar.select.windowRect(), Placement::Below);
		});
		addCommand(layerMenu, Command::NewLayer);
		addCommand(layerMenu, Command::SelectLayerAbove);
		addCommand(layerMenu, Command::SelectLayerBelow);
		addCommand(layerMenu, Command::SelectAllLayers);
		addCommand(layerMenu, Command::MoveLayerUp);
		addCommand(layerMenu, Command::MoveLayerDown);
		layerMenu.addSeparator();
		addCommand(layerMenu, Command::DuplicateLayer);
		addCommand(layerMenu, Command::RenameLayer);
		addCommand(layerMenu, Command::DeleteLayer);
		addCommand(layerMenu, Command::ClearLayer);
		addCommand(layerMenu, Command::MergeLayers);
		addCommand(layerMenu, Command::MergeAllLayers);
		layerMenu.addSeparator();
		addCommand(layerMenu, Command::ToggleLayerVisible);
		addCommand(layerMenu, Command::ToggleLayerClip);
		addCommand(layerMenu, Command::ToggleLayerLock);
		addCommand(layerMenu, Command::ToggleLayerAlphaLock);
		layerMenu.addSeparator();
		for (int4 mode = 0; mode < kBlendModeCount; mode++) {
			const auto blend = (BlendMode) mode;
			MenuEntry entry = MenuEntry::item(blendModeName(blend), [this, &document, blend]() {
				layerChanged(Command::NormalLayerBlend, document.setBlend(blend), "layer_blend");
				refreshFromState();
			});
			entry.isChecked = [&document, blend]() {
				return document.activeLayer() && document.activeLayer()->blend() == blend;
			};
			layerBlendMenu.add(std::move(entry));
		}
		layerBlendMenu.addSeparator();
		addCommand(layerBlendMenu, Command::PreviousLayerBlend);
		addCommand(layerBlendMenu, Command::NextLayerBlend);
		layerMenu.addSubmenu("Blend mode", layerBlendMenu);
		addCommand(layerOpacityMenu, Command::LayerOpacityDecrease);
		addCommand(layerOpacityMenu, Command::LayerOpacityIncrease);
		layerOpacityMenu.addSeparator();
		for (int4 tenth = 0; tenth <= 10; tenth++)
			addCommand(layerOpacityMenu, (Command) ((int4) Command::LayerOpacity0 + tenth));
		layerOpacityMenu.addSeparator();
		addCommand(layerOpacityMenu, Command::BakeLayerOpacity);
		layerMenu.addSubmenu("Opacity", layerOpacityMenu);

		historyMenu.setOnOpen([this](Menu& menu) { buildHistoryMenu(menu); });
		buildBrushMenu();
		buildColorMenu();
		debugMenu.add(
			MenuEntry::toggle(
				"Debug UI",
				[this]() { return isDebugUiShown && isDebugUiShown(); },
				[this]() {
					if (showDebugUi) showDebugUi(!(isDebugUiShown && isDebugUiShown()));
				}
			)
		);
		debugMenu.addToggle(
			"Show Tile Statistics",
			[&renderer]() { return renderer.debugOverlay(); },
			[&renderer]() { renderer.setDebugOverlay(!renderer.debugOverlay()); }
		);
		debugMenu.addToggle(
			"Desktop layout",
			[this]() { return isDesktopLayout && isDesktopLayout(); },
			[this]() {
				if (setDesktopLayout) setDesktopLayout(!(isDesktopLayout && isDesktopLayout()));
			}
		);
		debugMenu.add(MenuEntry::item("Simulate hang (5 s)", []() {
			TP_PROFILE_SCOPE("debug.simulated_hang");
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}));
		debugMenu.addSeparator();
		addCommand(debugMenu, Command::RecordSession);
		addCommand(debugMenu, Command::StopSessionRecording);
		assembleMainMenu();
	}

	void Sketch2DRoot::assembleMainMenu() {
		const std::pair<const char*, Menu*> menus[] = {
			{ "File", &fileMenu },   { "Edit", &editMenu },     { "View", &viewMenu },   { "Canvas", &canvasMenu },
			{ "Layer", &layerMenu }, { "Select", &selectMenu }, { "Line", &lineMenu },   { "History", &historyMenu },
			{ "Brush", &brushMenu }, { "Color", &colorMenu },   { "Debug", &debugMenu }, { "Help", &helpMenu },
		};
		for (const auto& [title, menu] : menus)
			mainMenu.addSubmenu(title, *menu);
		for (const auto& item : mainMenu.items())
			menuBar.addMenu(item->entry().text, *item->entry().submenu);
	}

	void Sketch2DRoot::setStabilizer(halnf amount) {
		if (canvas.settings().useBrushStabilizer) mBrush.setStabilizer(amount);
		else canvas.settings().globalStabilizer = std::clamp(amount, 0.0f, 1.0f);
		toolBar.brushPopup.refresh();
		notify("Stabilizer " + percentText(canvas.activeStabilizer()));
	}

	void Sketch2DRoot::onTick(halnf deltaMs) {
		library.advanceAutosave(deltaMs, canvas.isPainting());
		if (!library.hasUnsavedChanges()) stopTicking();
	}

	void Sketch2DRoot::defineSessionCommands() {
		commands.define(
			Command::RecordSession,
			[this](const Event*) { mSessionRecording->start(); },
			[this]() { return mSessionRecording && !mSessionRecording->isRecording(); }
		);
		commands.define(
			Command::StopSessionRecording,
			[this](const Event*) { mSessionRecording->stop(); },
			[this]() { return isRecordingSession(); }
		);
	}

	void Sketch2DRoot::setSessionRecording(ui::SessionRecordingControl* recording) {
		if (mSessionRecording) {
			toolBar.removeChild(mSessionRecording->badge);
			mSessionRecording->setOnBeforeStart({});
			mSessionRecording->setOnNotice({});
		}
		mSessionRecording = recording;
		if (!recording) return;

		const std::vector<Widget*>& items = toolBar.children();
		const auto undo = std::find(items.begin(), items.end(), &toolBar.undo);
		toolBar.insertChild((size_t) (undo - items.begin()), recording->badge);
		recording->setOnBeforeStart([this]() { canvas.cancelStroke(); });
		recording->setOnNotice([this](const std::string& message) { notify(message); });
		recording->recorder().setSnapshot([this](const SessionEnvironment& environment) -> SessionRecording::Saver {
			auto snapshot =
				std::make_shared<SessionSnapshot>(SessionSnapshot::capture(mDocument, mBrush, canvas.settings(), environment));
			return [snapshot](const std::string& directory) { return saveSessionSnapshot(directory, *snapshot); };
		});
	}

	std::optional<BrushParameter> Sketch2DRoot::heldBrushAdjustment() const {
		if (!ui()) return std::nullopt;
		const std::pair<Command, BrushParameter> holds[] = {
			{ Command::HoldBrushSize, BrushParameter::Size },
			{ Command::HoldBrushFlow, BrushParameter::Flow },
			{ Command::HoldBrushAngle, BrushParameter::Angle },
			{ Command::HoldBrushOpacity, BrushParameter::Opacity },
		};
		for (const auto& [command, parameter] : holds)
			for (const Shortcut& shortcut : commands.shortcuts().shortcutsFor(command))
				if (ui()->isKeyDown(shortcut.key)) return parameter;
		return std::nullopt;
	}

	void Sketch2DRoot::brushChanged(const std::string& message) {
		toolBar.brushPopup.refresh();
		toolBar.refreshFromState();
		canvas.requestPaint();
		if (!message.empty()) notify(message);
	}

	void Sketch2DRoot::defineBrushCommands() {
		commands.define(Command::SmallerBrush, [this](const Event*) {
			mBrush.scaleSize((100.0f - kBrushSizeStepPercent) / 100.0f);
			brushChanged(pixelsText(mBrush.size()));
		});
		commands.define(Command::BiggerBrush, [this](const Event*) {
			mBrush.scaleSize((100.0f + kBrushSizeStepPercent) / 100.0f);
			brushChanged(pixelsText(mBrush.size()));
		});
		const auto step = [this](Command command, BrushParameter parameter, halnf delta, bool percent) {
			commands.define(command, [this, parameter, delta, percent](const Event*) {
				mBrush.adjust(parameter, delta);
				const halnf value = mBrush.get(parameter);
				brushChanged(percent ? percentText(value) : std::to_string((int) std::round(value)) + "°");
			});
		};
		step(Command::BrushFlowDecrease, BrushParameter::Flow, -kBrushAmountStep, true);
		step(Command::BrushFlowIncrease, BrushParameter::Flow, kBrushAmountStep, true);
		step(Command::BrushOpacityDecrease, BrushParameter::Opacity, -kBrushAmountStep, true);
		step(Command::BrushOpacityIncrease, BrushParameter::Opacity, kBrushAmountStep, true);
		step(Command::BrushAngleDecrease, BrushParameter::Angle, -kBrushAngleStep, false);
		step(Command::BrushAngleIncrease, BrushParameter::Angle, kBrushAngleStep, false);
		for (int4 tenth = 1; tenth <= 10; tenth++) {
			const auto command = (Command) ((int4) Command::BrushOpacity10 + tenth - 1);
			const halnf opacity = (halnf) tenth / 10.0f;
			commands.define(command, [this, opacity](const Event*) {
				mBrush.setOpacity(opacity);
				brushChanged(percentText(opacity));
			});
		}
		for (const Command hold :
				 { Command::HoldBrushSize, Command::HoldBrushFlow, Command::HoldBrushAngle, Command::HoldBrushOpacity })
			commands.define(hold, [](const Event*) {});
		const auto tool = [this](Command command, BrushParameter parameter) {
			commands.define(
				command,
				[this, parameter](const Event*) {
					const bool active = canvas.adjustTool() == parameter;
					canvas.setAdjustTool(active ? std::nullopt : std::optional<BrushParameter>(parameter));
				},
				{},
				[this, parameter]() { return canvas.adjustTool() == parameter; }
			);
		};
		tool(Command::ToolBrushSize, BrushParameter::Size);
		tool(Command::ToolBrushAngle, BrushParameter::Angle);
		commands.define(Command::ResetBrush, [this](const Event*) {
			mBrush.resetEverything();
			brushChanged("Reset brush");
		});
		commands.define(Command::ResetAllBrushes, [this](const Event*) {
			toolStrip.resetBrushes();
			mBrush.resetEverything();
			brushChanged("Reset all brushes");
		});
		commands.define(Command::ShowBrushListPanel, [this](const Event*) { toolBar.brushTool.openPopup(); });
		commands.define(Command::ShowBrushOptionsPanel, [this](const Event*) { toolBar.brushSize.openPopup(); });
	}

	void Sketch2DRoot::pullBrushColor() {
		const RGBA current = colors.primary().rgba();
		const RGBA brush = mBrush.color;
		if (std::fabs(current.r - brush.r) + std::fabs(current.g - brush.g) + std::fabs(current.b - brush.b) > 1e-4f)
			colors.setPrimary(brush);
	}

	void Sketch2DRoot::colorChanged(const std::string& message) {
		mBrush.color = colors.primary().rgba();
		ensureNotErasing(mBrush);
		toolBar.colorPopup.setPrimary(colors.primary());
		toolBar.colorPopup.refresh();
		toolBar.refreshFromState();
		canvas.requestPaint();
		if (!message.empty()) notify(message);
	}

	void Sketch2DRoot::defineColorCommands() {
		const auto color = [this](Command command, std::function<void()> change) {
			commands.define(command, [this, command, change](const Event*) {
				pullBrushColor();
				change();
				colorChanged(commandInfo(command).label);
			});
		};
		color(Command::DefaultColors, [this]() { colors.resetToDefaults(); });
		color(Command::SwapColors, [this]() { colors.swap(); });
		color(Command::Cooler, [this]() { colors.cooler(); });
		color(Command::Warmer, [this]() { colors.warmer(); });
		color(Command::HueComplement, [this]() { colors.complement(); });
		color(Command::HueRotateLeft, [this]() { colors.rotateHue(-ColorPair::kHueStep); });
		color(Command::HueRotateRight, [this]() { colors.rotateHue(ColorPair::kHueStep); });
		color(Command::Grayer, [this]() { colors.changeSaturation(-ColorPair::kAmountStep); });
		color(Command::Purer, [this]() { colors.changeSaturation(ColorPair::kAmountStep); });
		color(Command::Darker, [this]() { colors.changeValue(-ColorPair::kAmountStep); });
		color(Command::Brighter, [this]() { colors.changeValue(ColorPair::kAmountStep); });
		for (int4 tenth = 0; tenth <= 10; tenth++) {
			const auto command = (Command) ((int4) Command::Lightness0 + tenth);
			commands.define(command, [this, tenth](const Event*) {
				pullBrushColor();
				const bool fine = tenth > 0 && tenth < 10 && commands.wasRecent(Command::Lightness100);
				colors.setValue((halnf) tenth / (fine ? 100.0f : 10.0f));
				colorChanged("Lightness " + percentText(colors.primary().value));
			});
		}
		commands.define(Command::AddSwatch, [this](const Event*) {
			pullBrushColor();
			const RGBA color = colors.primary().rgba();
			if (!swatches.add(color)) {
				notify(swatches.colors().size() >= SwatchSet::kCapacity ? "Swatches are full" : "Already in swatches");
				return;
			}
			if (!swatchPath.empty()) swatches.save(swatchPath);
			toolBar.colorPopup.showSwatches(swatches);
			notify("Added " + colorHex(color) + " to swatches");
		});
		commands.define(Command::ClearColorHistory, [this](const Event*) {
			colorHistory.clear();
			toolBar.colorPopup.showHistory(colorHistory);
			notify("Cleared color history");
		});
		commands.define(Command::CopyColor, [this](const Event*) {
			pullBrushColor();
			const std::string hex = colorHex(colors.primary().rgba());
			if (ui() && ui()->windowHost()) ui()->windowHost()->setClipboardText(hex);
			notify("Copied " + hex);
		});
		commands.define(Command::PasteColor, [this](const Event*) {
			const std::string text = ui() && ui()->windowHost() ? ui()->windowHost()->clipboardText() : std::string();
			const auto parsed = parseColorHex(text);
			if (!parsed) {
				notify("The clipboard has no color");
				return;
			}
			colors.setPrimary(*parsed);
			colorChanged("Pasted " + colorHex(*parsed));
		});
	}

	void Sketch2DRoot::buildColorMenu() {
		addCommand(colorMenu, Command::DefaultColors);
		addCommand(colorMenu, Command::SwapColors);
		colorMenu.addSeparator();
		addCommand(colorMenu, Command::CopyColor);
		addCommand(colorMenu, Command::PasteColor);
		addCommand(colorMenu, Command::AddSwatch);
		addCommand(colorMenu, Command::ClearColorHistory);
		colorMenu.addSeparator();
		addCommand(colorMenu, Command::Darker);
		addCommand(colorMenu, Command::Brighter);
		addCommand(colorMenu, Command::Grayer);
		addCommand(colorMenu, Command::Purer);
		addCommand(colorMenu, Command::Cooler);
		addCommand(colorMenu, Command::Warmer);
		colorMenu.addSeparator();
		addCommand(colorMenu, Command::HueComplement);
		addCommand(colorMenu, Command::HueRotateLeft);
		addCommand(colorMenu, Command::HueRotateRight);
		for (int4 tenth = 0; tenth <= 10; tenth++)
			addCommand(lightnessMenu, (Command) ((int4) Command::Lightness0 + tenth));
		colorMenu.addSubmenu("Lightness", lightnessMenu);

		addCommand(pickerMenu, Command::ToolPicker);
		pickerMenu.addSeparator();
		PickerSettings& picker = canvas.settings().picker;
		pickerMenu.add(
			MenuEntry::toggle(
				"Sample all layers",
				[&picker]() { return picker.sampleAllLayers; },
				[&picker]() { picker.sampleAllLayers = !picker.sampleAllLayers; }
			)
		);
		pickerMenu.add(
			MenuEntry::toggle(
				"Sample opacity",
				[&picker]() { return picker.sampleOpacity; },
				[&picker]() { picker.sampleOpacity = !picker.sampleOpacity; }
			)
		);
		pickerMenu.addSeparator();
		const std::pair<const char*, PickerStyle> styles[] = {
			{ "Cursor", PickerStyle::Cursor },
			{ "Ring", PickerStyle::Ring },
			{ "Balloon", PickerStyle::Balloon },
			{ "Magnifier", PickerStyle::Magnifier },
		};
		for (const auto& [name, style] : styles) {
			const PickerStyle value = style;
			pickerMenu.add(
				MenuEntry::toggle(
					name, [&picker, value]() { return picker.style == value; }, [&picker, value]() { picker.style = value; }
				)
			);
		}
		colorMenu.addSeparator();
		colorMenu.addSubmenu("Color picker", pickerMenu);

		addCommand(bucketMenu, Command::ToolBucket);
		bucketMenu.addSeparator();
		BucketSettings& bucket = canvas.settings().bucket;
		bucketMenu.add(
			MenuEntry::toggle(
				"Sample all visible layers",
				[&bucket]() { return bucket.allLayers; },
				[&bucket]() { bucket.allLayers = !bucket.allLayers; }
			)
		);
		bucketMenu.add(
			MenuEntry::toggle(
				"Sample only alpha channel",
				[&bucket]() { return bucket.onlyAlpha; },
				[&bucket]() { bucket.onlyAlpha = !bucket.onlyAlpha; }
			)
		);
		bucketMenu.addSeparator();
		for (const int4 percent : { 0, 5, 10, 20, 35, 50 }) {
			const halnf value = (halnf) percent / 100.0f;
			bucketMenu.add(
				MenuEntry::toggle(
					"Threshold " + std::to_string(percent) + "%",
					[&bucket, value]() { return std::abs(bucket.threshold - value) < 1e-4f; },
					[&bucket, value]() { bucket.threshold = value; }
				)
			);
		}
		bucketMenu.addSeparator();
		for (const int4 pixels : { 0, 1, 2, 4 }) {
			bucketMenu.add(
				MenuEntry::toggle(
					"Expand " + std::to_string(pixels) + " px",
					[&bucket, pixels]() { return bucket.spread == pixels; },
					[&bucket, pixels]() { bucket.spread = pixels; }
				)
			);
		}
		colorMenu.addSubmenu("Paint bucket", bucketMenu);

		addCommand(gradientMenu, Command::ToolGradient);
		gradientMenu.addSeparator();
		GradientSettings& gradient = canvas.settings().gradient;
		gradientMenu.add(
			MenuEntry::toggle(
				"Linear",
				[&gradient]() { return gradient.shape == GradientShape::Linear; },
				[&gradient]() { gradient.shape = GradientShape::Linear; }
			)
		);
		gradientMenu.add(
			MenuEntry::toggle(
				"Radial",
				[&gradient]() { return gradient.shape == GradientShape::Radial; },
				[&gradient]() { gradient.shape = GradientShape::Radial; }
			)
		);
		gradientMenu.addSeparator();
		gradientMenu.add(
			MenuEntry::toggle(
				"Color to transparent",
				[&gradient]() { return gradient.toTransparent; },
				[&gradient]() { gradient.toTransparent = !gradient.toTransparent; }
			)
		);
		gradientMenu.add(
			MenuEntry::toggle(
				"Eraser", [&gradient]() { return gradient.eraser; }, [&gradient]() { gradient.eraser = !gradient.eraser; }
			)
		);
		colorMenu.addSubmenu("Gradient", gradientMenu);

		LineSettings& line = canvas.settings().line;
		const auto lineMode = [this, &line](Command command, LineMode mode) {
			commands.define(
				command,
				[this, &line, mode](const Event*) {
					line.mode = mode;
					refreshFromState();
				},
				{},
				[&line, mode]() { return line.mode == mode; }
			);
			addCommand(lineMenu, command);
		};
		lineMode(Command::LineNormal, LineMode::Normal);
		lineMode(Command::LineRuler, LineMode::Ruler);
		lineMode(Command::LineEllipse, LineMode::Ellipse);
		lineMode(Command::LinePerspective, LineMode::Perspective);
		PerspectiveSettings& perspective = canvas.settings().perspective;
		commands.define(
			Command::LineIsometric,
			[this, &line, &perspective](const Event*) {
				line.mode = LineMode::Perspective;
				perspective.type = PerspectiveType::Isometric;
				if (!perspective.placed) canvas.resetPerspective();
				canvas.requestPaint();
			},
			{},
			[&line, &perspective]() {
				return line.mode == LineMode::Perspective && perspective.type == PerspectiveType::Isometric;
			}
		);
		addCommand(lineMenu, Command::LineIsometric);
		const auto perspectiveType = [this, &perspective](const std::string& text, PerspectiveType type) {
			perspectiveMenu.add(
				MenuEntry::toggle(
					text,
					[&perspective, type]() { return perspective.type == type; },
					[this, &perspective, type]() {
						if (perspective.locked) {
							notify("The perspective is locked");
							return;
						}
						perspective.type = type;
						canvas.resetPerspective();
					}
				)
			);
		};
		perspectiveType("1 point", PerspectiveType::OnePoint);
		perspectiveType("2 point", PerspectiveType::TwoPoint);
		perspectiveType("3 point", PerspectiveType::ThreePoint);
		perspectiveType("Isometric", PerspectiveType::Isometric);
		perspectiveType("Radial", PerspectiveType::Radial);
		perspectiveMenu.addSeparator();
		for (const halnf degrees : { 26.565f, 30.0f, 45.0f }) {
			char label[32];
			std::snprintf(label, sizeof label, "Isometric angle %.1f°", degrees);
			perspectiveMenu.add(
				MenuEntry::toggle(
					label,
					[&perspective, degrees]() { return std::abs(perspective.isometricDegrees - degrees) < 0.01f; },
					[this, &perspective, degrees]() {
						perspective.isometricDegrees = degrees;
						canvas.requestPaint();
					}
				)
			);
		}
		perspectiveMenu.addSeparator();
		commands.define(Command::MovePerspectivePoint, [this, &perspective](const Event*) {
			if (perspective.locked) {
				notify("The perspective is locked");
				return;
			}
			canvas.armPerspectiveMove();
			notify("Drag a vanishing point");
		});
		commands.define(
			Command::LockPerspective,
			[&perspective](const Event*) { perspective.locked = !perspective.locked; },
			{},
			[&perspective]() { return perspective.locked; }
		);
		commands.define(Command::ResetPerspective, [this, &perspective](const Event*) {
			if (!perspective.locked) canvas.resetPerspective();
		});
		addCommand(perspectiveMenu, Command::MovePerspectivePoint);
		addCommand(perspectiveMenu, Command::LockPerspective);
		addCommand(perspectiveMenu, Command::ResetPerspective);
		lineMenu.addSubmenu("Perspective", perspectiveMenu);
		lineMenu.addSeparator();
		SymmetrySettings& symmetry = canvas.settings().symmetry;
		commands.define(
			Command::LineSymmetry,
			[this, &symmetry](const Event*) {
				symmetry.enabled = !symmetry.enabled;
				if (symmetry.enabled && symmetry.center.x == 0 && symmetry.center.y == 0) canvas.resetSymmetry();
				canvas.requestPaint();
			},
			{},
			[&symmetry]() { return symmetry.enabled; }
		);
		commands.define(
			Command::ShowSymmetryLine,
			[this, &symmetry](const Event*) {
				symmetry.visible = !symmetry.visible;
				canvas.requestPaint();
			},
			[&symmetry]() { return symmetry.enabled; },
			[&symmetry]() { return symmetry.visible; }
		);
		commands.define(
			Command::MoveSymmetryLine,
			[this, &symmetry](const Event*) {
				if (symmetry.locked) {
					notify("The symmetry line is locked");
					return;
				}
				canvas.armSymmetryMove();
				notify("Drag to move the symmetry line");
			},
			[&symmetry]() { return symmetry.enabled; }
		);
		commands.define(
			Command::LockSymmetryLine,
			[&symmetry](const Event*) { symmetry.locked = !symmetry.locked; },
			[&symmetry]() { return symmetry.enabled; },
			[&symmetry]() { return symmetry.locked; }
		);
		commands.define(
			Command::ResetSymmetryLine,
			[this, &symmetry](const Event*) {
				if (!symmetry.locked) canvas.resetSymmetry();
			},
			[&symmetry]() { return symmetry.enabled; }
		);
		addCommand(symmetryMenu, Command::LineSymmetry);
		addCommand(symmetryMenu, Command::ShowSymmetryLine);
		addCommand(symmetryMenu, Command::MoveSymmetryLine);
		addCommand(symmetryMenu, Command::LockSymmetryLine);
		addCommand(symmetryMenu, Command::ResetSymmetryLine);
		lineMenu.addSubmenu("Symmetry", symmetryMenu);
		lineMenu.addSeparator();
		lineMenu.add(
			MenuEntry::toggle(
				"Hold still for line",
				[&line]() { return line.holdForLine; },
				[&line]() { line.holdForLine = !line.holdForLine; }
			)
		);
		for (const halnf seconds : { 0.3f, 0.5f, 0.8f, 1.2f }) {
			char label[32];
			std::snprintf(label, sizeof label, "Hold time %.1f s", seconds);
			lineMenu.add(
				MenuEntry::toggle(
					label,
					[&line, seconds]() { return std::abs(line.holdSeconds - seconds) < 1e-3f; },
					[&line, seconds]() { line.holdSeconds = seconds; }
				)
			);
		}
		toolBar.line.setOnActivate([this]() {
			lineMenu.openAt(toolBar.line, toolBar.line.windowRect(), Placement::Below);
		});

		addCommand(moveMenu, Command::ToolMove);
		moveMenu.addSeparator();
		MoveSettings& move = canvas.settings().move;
		moveMenu.add(
			MenuEntry::toggle("Move pixels", [&move]() { return !move.duplicate; }, [&move]() { move.duplicate = false; })
		);
		moveMenu.add(
			MenuEntry::toggle("Duplicate pixels", [&move]() { return move.duplicate; }, [&move]() { move.duplicate = true; })
		);
		layerMenu.addSeparator();
		layerMenu.addSubmenu("Move tool", moveMenu);
	}

	void Sketch2DRoot::buildBrushMenu() {
		addCommand(brushMenu, Command::ResetAllBrushes);
		brushMenu.addSeparator();
		addCommand(brushMenu, Command::SmallerBrush);
		addCommand(brushMenu, Command::BiggerBrush);
		brushMenu.addSeparator();
		const auto reset = [this](const char* label, void (Brush::*action)()) {
			resetMenu.addItem(label, [this, action, label]() {
				(mBrush.*action)();
				brushChanged(label);
			});
		};
		reset("Reset all brushes (everything)", &Brush::resetEverything);
		reset("Reset all brush parameters", &Brush::resetParameters);
		reset("Reset all brush stabilization", &Brush::resetStabilization);
		reset("Reset all brush textures", &Brush::resetTextures);
		brushMenu.addSubmenu("Reset brush", resetMenu);
		brushMenu.addSeparator();
		addCommand(brushMenu, Command::ShowBrushListPanel);
		addCommand(brushMenu, Command::ShowBrushOptionsPanel);
		brushMenu.addSeparator();
		addCommand(adjustMenu, Command::BrushFlowDecrease);
		addCommand(adjustMenu, Command::BrushFlowIncrease);
		addCommand(adjustMenu, Command::BrushOpacityDecrease);
		addCommand(adjustMenu, Command::BrushOpacityIncrease);
		addCommand(adjustMenu, Command::BrushAngleDecrease);
		addCommand(adjustMenu, Command::BrushAngleIncrease);
		adjustMenu.addSeparator();
		addCommand(adjustMenu, Command::ToolBrushSize);
		addCommand(adjustMenu, Command::ToolBrushAngle);
		brushMenu.addSubmenu("Adjust", adjustMenu);
		for (int4 tenth = 1; tenth <= 10; tenth++)
			addCommand(brushOpacityMenu, (Command) ((int4) Command::BrushOpacity10 + tenth - 1));
		brushMenu.addSubmenu("Opacity", brushOpacityMenu);
		brushMenu.addSeparator();

		for (int4 step = 0; step <= 10; step++) {
			const halnf amount = (halnf) step / 10.0f;
			MenuEntry entry = MenuEntry::item(step == 0 ? std::string("Off") : percentText(amount), [this, amount]() {
				setStabilizer(amount);
			});
			entry.isChecked = [this, amount]() { return std::fabs(canvas.activeStabilizer() - amount) < 0.05f; };
			stabilizerMenu.add(std::move(entry));
		}
		stabilizerMenu.addSeparator();
		addCommand(stabilizerMenu, Command::StabilizerDecrease);
		addCommand(stabilizerMenu, Command::StabilizerIncrease);
		stabilizerMenu.addSeparator();
		stabilizerMenu.addToggle(
			"Use brush stabilizer",
			[this]() { return canvas.settings().useBrushStabilizer; },
			[this]() {
				canvas.settings().useBrushStabilizer = !canvas.settings().useBrushStabilizer;
				toolBar.brushPopup.refresh();
			}
		);
		stabilizerMenu.addItem("Reset all brush stabilization", [this]() {
			mBrush.setStabilizer(Brush::kDefaultStabilizer);
			canvas.settings().globalStabilizer = Brush::kDefaultStabilizer;
			toolBar.brushPopup.refresh();
		});
		brushMenu.addSubmenu("Stabilizer", stabilizerMenu);
	}

	void Sketch2DRoot::buildCursorMenu() {
		BrushCursorSettings& cursor = canvas.settings().cursor;
		for (int4 index = 0; index < kBrushCursorStyleCount; index++) {
			const BrushCursorStyle style = (BrushCursorStyle) index;
			cursorMenu.addToggle(
				brushCursorStyleName(style),
				[&cursor, style]() { return cursor.style == style; },
				[this, &cursor, style]() {
					cursor.style = style;
					canvas.refreshCursor();
					canvas.requestPaint();
				}
			);
		}
		cursorMenu.addSeparator();
		const auto toggle = [this](const char* label, bool BrushCursorSettings::* flag) {
			cursorMenu.addToggle(
				label,
				[this, flag]() { return canvas.settings().cursor.*flag; },
				[this, flag]() {
					canvas.settings().cursor.*flag = !(canvas.settings().cursor.*flag);
					canvas.refreshCursor();
					canvas.requestPaint();
				}
			);
		};
		toggle("Show contour for eraser & smudge", &BrushCursorSettings::contourForEraserAndSmudge);
		toggle("Show crosshair for small contour", &BrushCursorSettings::crosshairForSmallContour);
		toggle("Show contour spotlight", &BrushCursorSettings::spotlight);
		toggle("Show cursor for pen", &BrushCursorSettings::showForPen);
	}

}
