#include "Slider.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace tp;
using namespace tp::ui;

namespace {
	halnf clampValue(halnf value, halnf minimum, halnf maximum) {
		if (value < minimum) return minimum;
		if (value > maximum) return maximum;
		return value;
	}

	std::string formatTwoDecimals(halnf value) {
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "%.2f", value);
		return buffer;
	}
}

Slider::Slider(halnf minimum, halnf maximum, halnf value) :
	mMinimum(minimum),
	mMaximum(maximum > minimum ? maximum : minimum),
	mValue(clampValue(value, mMinimum, mMaximum)) {
	setDebugName("slider");
}

void Slider::setRange(halnf minimum, halnf maximum) {
	mMinimum = minimum;
	mMaximum = maximum > minimum ? maximum : minimum;
	setValue(mValue);
	requestPaint();
}

void Slider::setValue(halnf value) {
	const halnf next = snapped(clampValue(value, mMinimum, mMaximum));
	if (next == mValue) return;

	mValue = next;
	requestPaint();
	if (mOnValueShown) mOnValueShown();
}

void Slider::changeValue(halnf value) {
	const halnf previous = mValue;
	setValue(value);
	if (mValue != previous && mOnChanged) mOnChanged(mValue);
}

std::vector<halnf> tp::ui::groupTickMarks(std::vector<halnf> fractions, halnf minimumGap) {
	std::sort(fractions.begin(), fractions.end());

	std::vector<halnf> grouped;
	for (halnf fraction : fractions) {
		const halnf clamped = clampValue(fraction, 0, 1);
		if (!grouped.empty() && clamped - grouped.back() < minimumGap) continue;
		grouped.push_back(clamped);
	}
	return grouped;
}

void Slider::setTickMarks(std::vector<halnf> fractions) {
	mTickMarks = std::move(fractions);
	requestPaint();
}

std::vector<halnf> Slider::visibleTickMarks() const {
	const RectF track = trackRect();
	const halnf length = isVertical() ? track.size.y : track.size.x;
	if (length <= 0) return {};
	return groupTickMarks(mTickMarks, theme().metrics.sliderTickSpacing / length);
}

void Slider::paintTickMarks(Painter& painter, const RectF& track) const {
	const Metrics& metrics = theme().metrics;
	const halnf half = metrics.sliderTickLength / 2;
	const RGBA color = theme().palette.mutedText;

	for (halnf fraction : visibleTickMarks()) {
		const Vec2F center = pointOnTrack(track, fraction);
		const Vec2F across = isVertical() ? Vec2F(half, 0.0f) : Vec2F(0.0f, half);
		painter.line(center - across, center + across, color, metrics.sliderTickThickness);
	}
}

void Slider::setStep(halnf step) {
	mStep = step > 0 ? step : 0;
	setValue(mValue);
}

void Slider::setScale(Scale scale) {
	mScale = scale;
	requestPaint();
}

void Slider::setOrientation(Axis orientation) {
	if (orientation == mOrientation) return;
	mOrientation = orientation;
	requestLayout();
	requestPaint();
}

bool Slider::isLogarithmic() const { return mScale == Scale::Logarithmic && mMinimum > 0 && mMaximum > mMinimum; }

halnf Slider::fractionForValue(halnf value) const {
	const halnf span = mMaximum - mMinimum;
	if (span <= 0) return 0;
	const halnf clamped = clampValue(value, mMinimum, mMaximum);
	if (!isLogarithmic()) return (clamped - mMinimum) / span;
	return std::log(clamped / mMinimum) / std::log(mMaximum / mMinimum);
}

halnf Slider::valueForFraction(halnf fraction) const {
	const halnf amount = clampValue(fraction, 0, 1);
	if (!isLogarithmic()) return mMinimum + (mMaximum - mMinimum) * amount;
	return mMinimum * std::pow(mMaximum / mMinimum, amount);
}

halnf Slider::fraction() const { return fractionForValue(mValue); }

Extent Slider::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	if (axis == mOrientation) return Extent::shrinkable(metrics.sliderHandleSize * 8, metrics.sliderHandleSize * 2);
	return Extent::natural(metrics.sliderHandleSize + 4);
}

void Slider::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary) return;
			mDragging = true;
			ui()->capturePointer(*this);
			if (mOnDragging) mOnDragging(true);
			changeValue(valueAt(event.position()));
			event.accept();
			break;

		case Event::Type::PointerMove:
			if (!mDragging) return;
			changeValue(valueAt(event.position()));
			event.accept();
			break;

		case Event::Type::PointerUp:
			if (!mDragging || event.button() != PointerButton::Primary) return;
			mDragging = false;
			ui()->releasePointer(*this);
			if (mOnDragging) mOnDragging(false);
			event.accept();
			break;

		case Event::Type::KeyDown:
			if (event.keyCode() == InputID::LEFT || event.keyCode() == InputID::DOWN) stepBy(-1);
			else if (event.keyCode() == InputID::RIGHT || event.keyCode() == InputID::UP) stepBy(1);
			else return;
			event.accept();
			break;

		default: Control::onEvent(event); return;
	}

	refreshVisualState();
}

void Slider::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF track = trackRect();
	const halnf radius = std::min(track.size.x, track.size.y) / 2;

	const std::vector<RGBA> stops = trackGradient();
	if (stops.size() >= 2) {
		const halnf length = isVertical() ? track.size.y : track.size.x;
		const halnf segment = length / (halnf) (stops.size() - 1);
		for (size_t index = 0; index + 1 < stops.size(); index++) {
			if (isVertical()) {
				const halnf bottom = track.pos.y + track.size.y - segment * (halnf) (index + 1);
				const RectF part = makeRect(track.pos.x, bottom, track.size.x, segment);
				painter.fillGradientRect(part, stops[index + 1], stops[index], false);
			} else {
				const RectF part = makeRect(track.pos.x + segment * (halnf) index, track.pos.y, segment, track.size.y);
				painter.fillGradientRect(part, stops[index], stops[index + 1], true);
			}
		}
		painter.strokeRect(track, style.palette.controlBorder, 0, 1);
	} else {
		painter.fillRect(track, style.palette.track, radius);
		const RectF filled =
			isVertical()
				? makeRect(track.pos.x, track.pos.y + track.size.y * (1 - fraction()), track.size.x, track.size.y * fraction())
				: makeRect(track.pos, { track.size.x * fraction(), track.size.y });
		painter.fillRect(filled, style.palette.sliderFill, radius);
	}
	paintTickMarks(painter, track);

	const halnf reveal = mHandleReveal.value();
	if (reveal <= 0.01f) return;

	const halnf handleRadius = style.metrics.sliderHandleSize / 2 * reveal;
	const Vec2F handleCenter = pointOnTrack(track, fraction());
	const RGBA ring = mDragging ? style.palette.mutedText : style.palette.controlBorder;

	if (showsFocusRing()) painter.strokeCircle(handleCenter, handleRadius + 3, style.palette.focusRing, 2);
	painter.fillCircle(handleCenter + Vec2F(0.0f, 0.5f), handleRadius, style.palette.shadow);
	painter.fillCircle(handleCenter, handleRadius, style.palette.text);
	painter.strokeCircle(handleCenter, handleRadius, ring, 1.5f);
}

bool Slider::showsHandle() const { return isHovered() || mDragging || showsFocusRing(); }

void Slider::refreshVisualState() {
	animate(mHandleReveal, showsHandle() ? 1.0f : 0.0f);
	Control::refreshVisualState();
}

RectF Slider::trackRect() const {
	const Metrics& metrics = theme().metrics;
	const halnf inset = metrics.sliderHandleSize / 2;
	const halnf thickness = mTrackGradient ? metrics.sliderHandleSize : metrics.sliderTrackThickness;
	if (isVertical()) return makeRect((size().x - thickness) / 2, inset, thickness, size().y - inset * 2);
	return makeRect(inset, (size().y - thickness) / 2, size().x - inset * 2, thickness);
}

Vec2F Slider::pointOnTrack(const RectF& track, halnf amount) const {
	if (isVertical()) return { size().x / 2, track.pos.y + track.size.y * (1 - amount) };
	return { track.pos.x + track.size.x * amount, size().y / 2 };
}

Vec2F Slider::windowPointForValue(halnf value) const {
	const RectF track = trackRect();
	return windowPosition() + pointOnTrack(track, fractionForValue(value));
}

halnf Slider::valueAt(Vec2F local) const {
	const RectF track = trackRect();
	if (isVertical()) {
		const halnf amount = track.size.y > 0 ? clampValue((local.y - track.pos.y) / track.size.y, 0, 1) : 0;
		return valueForFraction(1 - amount);
	}
	const halnf amount = track.size.x > 0 ? clampValue((local.x - track.pos.x) / track.size.x, 0, 1) : 0;
	return valueForFraction(amount);
}

halnf Slider::snapped(halnf value) const {
	if (mStep <= 0) return value;
	return clampValue(mMinimum + std::round((value - mMinimum) / mStep) * mStep, mMinimum, mMaximum);
}

void Slider::stepBy(halnf steps) {
	if (mStep <= 0 && isLogarithmic()) {
		changeValue(valueForFraction(fraction() + steps / 100));
		return;
	}
	const halnf step = mStep > 0 ? mStep : (mMaximum - mMinimum) / 100;
	changeValue(mValue + step * steps);
}

LabeledSlider::LabeledSlider(std::string name, halnf minimum, halnf maximum, halnf value) :
	mName(std::move(name)),
	mSlider(minimum, maximum, value),
	mFormatter(formatTwoDecimals) {
	setDebugName("labeled slider");

	emplaceLayout<ColumnLayout>(2);
	mHeader.emplaceLayout<RowLayout>(theme().metrics.spacing);

	mName.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mValueText.setAlignment(TextAlign::End);
	mValueText.setTone(Label::Tone::Muted);

	mHeader.addChild(mName);
	mHeader.addChild(mValueText);
	addChild(mHeader);
	addChild(mSlider);

	mSlider.setOnValueShown([this]() { refreshValueText(); });
	mSlider.setOnChanged([this](halnf changed) {
		if (mOnChanged) mOnChanged(changed);
	});
	refreshValueText();
}

void LabeledSlider::setValueFormatter(std::function<std::string(halnf)> formatter) {
	mFormatter = std::move(formatter);
	refreshValueText();
}

void LabeledSlider::setOnChanged(std::function<void(halnf)> onChanged) { mOnChanged = std::move(onChanged); }

void LabeledSlider::refreshValueText() { mValueText.setText(mFormatter ? mFormatter(mSlider.value()) : std::string()); }

ProgressBar::ProgressBar() :
	mFraction(0, Motion::smooth()) {
	setDebugName("progress");
}

void ProgressBar::setFraction(halnf fraction) {
	mFraction.animateTo(clampValue(fraction, 0, 1));
	startTicking();
}

Extent ProgressBar::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	if (axis == Axis::Horizontal) return Extent::shrinkable(metrics.sliderHandleSize * 8, metrics.sliderHandleSize);
	return Extent::natural(metrics.sliderTrackThickness * 1.5f);
}

void ProgressBar::onTick(halnf deltaMs) {
	if (!mFraction.advance(deltaMs)) stopTicking();
	requestPaint();
}

void ProgressBar::paint(Painter& painter) {
	const Theme& style = theme();
	const halnf radius = size().y / 2;

	painter.fillRect(localRect(), style.palette.track, radius);
	painter.fillRect(makeRect({ 0, 0 }, { size().x * mFraction.value(), size().y }), style.palette.sliderFill, radius);
}
