#include "Toggles.hpp"

#include "Theme.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

namespace {
	RGBA mix(const RGBA& from, const RGBA& to, halnf amount) { return Palette::mix(from, to, amount); }

	RGBA withAlpha(const RGBA& color, halnf alpha) { return { color.r, color.g, color.b, color.a * alpha }; }
}

ToggleControl::ToggleControl(std::string text) :
	mText(std::move(text)),
	mCheckedAmount(0) {}

void ToggleControl::setChecked(bool checked) {
	if (checked == mChecked) return;

	mChecked = checked;
	animate(mCheckedAmount, checked ? 1.0f : 0.0f);
	if (mOnChanged) mOnChanged(checked);
}

void ToggleControl::setText(std::string text) {
	mText = std::move(text);
	requestLayout();
}

Extent ToggleControl::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;

	if (axis == Axis::Vertical) return Extent::natural(metrics.controlHeight);

	const halnf label = mText.empty() ? 0 : metrics.spacing + textWidth(mText, metrics.fontSize);
	return Extent::natural(indicatorWidth() + label);
}

void ToggleControl::onActivated() {
	if (togglesOnActivate()) setChecked(!mChecked);
}

void ToggleControl::paint(Painter& painter) {
	const Theme& style = theme();

	paintIndicator(painter, indicatorBox(), mCheckedAmount.value());

	if (mText.empty()) return;

	const halnf labelLeft = indicatorWidth() + style.metrics.spacing;
	const RectF labelBox = makeRect(labelLeft, 0, size().x - labelLeft, size().y);
	const RGBA color = isEffectivelyEnabled() ? style.palette.text : style.palette.disabledText;
	painter.text(mText, labelBox, style.metrics.fontSize, TextAlign::Start, color);
}

halnf ToggleControl::indicatorWidth() const { return theme().metrics.indicatorSize; }

RectF ToggleControl::indicatorBox() const {
	const halnf height = theme().metrics.indicatorSize;
	return makeRect(0, (size().y - height) / 2, indicatorWidth(), height);
}

Checkbox::Checkbox(std::string text) :
	ToggleControl(std::move(text)) {
	setDebugName("checkbox");
}

void Checkbox::paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) {
	const Palette& palette = theme().palette;
	const halnf radius = theme().metrics.indicatorCornerRadius + 1;
	const RGBA uncheckedFill = isHovered() ? palette.controlHovered : palette.field;
	const RGBA checkedFill = isHovered() ? palette.toggleOnHovered : palette.toggleOn;
	const RGBA uncheckedBorder = isHovered() ? palette.mutedText : palette.controlBorder;

	if (showsFocusRing()) painter.strokeRect(insetRect(box, -3), palette.focusRing, radius + 3, 2);
	painter.fillRect(box, mix(uncheckedFill, checkedFill, checkedAmount), radius);
	painter.strokeRect(box, withAlpha(uncheckedBorder, 1 - checkedAmount), radius);

	if (checkedAmount <= 0.01f) return;

	const Vec2F origin = box.pos;
	const halnf unit = box.size.x;
	const halnf grow = 0.6f + 0.4f * checkedAmount;
	const Vec2F center = box.center();
	const auto point = [&](halnf x, halnf y) { return center + (origin + Vec2F(unit * x, unit * y) - center) * grow; };
	painter.polyline(
		{ point(0.27f, 0.52f), point(0.44f, 0.68f), point(0.74f, 0.35f) }, withAlpha(palette.onToggle, checkedAmount), 2
	);
}

Switch::Switch(std::string text) :
	ToggleControl(std::move(text)) {
	setDebugName("switch");
}

void Switch::paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) {
	const Palette& palette = theme().palette;
	const halnf radius = box.size.y / 2;
	const RGBA offTrack = isHovered() ? palette.controlPressed : palette.track;
	const RGBA onTrack = isHovered() ? palette.toggleOnHovered : palette.toggleOn;

	if (showsFocusRing()) painter.strokeRect(insetRect(box, -3), palette.focusRing, radius + 3, 2);
	painter.fillRect(box, mix(offTrack, onTrack, checkedAmount), radius);

	const halnf knobRadius = radius - 3;
	const halnf travel = box.size.x - box.size.y;
	const Vec2F knobCenter = { box.pos.x + radius + travel * checkedAmount, box.pos.y + radius };
	const RGBA offKnob = palette.mutedText;
	painter.fillCircle(knobCenter, knobRadius, mix(offKnob, palette.onToggle, checkedAmount));
}

halnf Switch::indicatorWidth() const { return theme().metrics.indicatorSize * 1.9f; }

RadioButton::RadioButton(std::string text) :
	ToggleControl(std::move(text)) {
	setDebugName("radio");
}

RadioButton::~RadioButton() {
	if (mGroup) mGroup->remove(*this);
}

void RadioButton::paintIndicator(Painter& painter, const RectF& box, halnf checkedAmount) {
	const Palette& palette = theme().palette;
	const Vec2F center = box.center();
	const halnf radius = box.size.y / 2;
	const RGBA uncheckedFill = isHovered() ? palette.controlHovered : palette.field;
	const RGBA checkedFill = isHovered() ? palette.toggleOnHovered : palette.toggleOn;
	const RGBA uncheckedBorder = isHovered() ? palette.mutedText : palette.controlBorder;

	if (showsFocusRing()) painter.strokeCircle(center, radius + 3, palette.focusRing, 2);
	painter.fillCircle(center, radius, mix(uncheckedFill, checkedFill, checkedAmount));
	painter.strokeCircle(center, radius, withAlpha(uncheckedBorder, 1 - checkedAmount));

	if (checkedAmount > 0.01f) painter.fillCircle(center, radius * 0.4f * checkedAmount, palette.onToggle);
}

void RadioButton::onActivated() {
	if (mGroup) mGroup->select(*this);
	else setChecked(true);
}

RadioGroup::~RadioGroup() {
	for (RadioButton* button : mButtons)
		button->mGroup = nullptr;
}

void RadioGroup::add(RadioButton& button) {
	if (button.mGroup) button.mGroup->remove(button);

	button.mGroup = this;
	mButtons.push_back(&button);
}

void RadioGroup::remove(RadioButton& button) {
	mButtons.erase(std::remove(mButtons.begin(), mButtons.end(), &button), mButtons.end());
	button.mGroup = nullptr;
}

void RadioGroup::select(RadioButton& button) {
	if (button.mGroup != this) return;
	if (button.isChecked()) return;

	for (RadioButton* other : mButtons)
		other->setChecked(other == &button);
	if (mOnChanged) mOnChanged((size_t) selectedIndex());
}

void RadioGroup::selectIndex(size_t index) {
	if (index < mButtons.size()) select(*mButtons[index]);
}

long RadioGroup::selectedIndex() const {
	for (size_t index = 0; index < mButtons.size(); index++) {
		if (mButtons[index]->isChecked()) return (long) index;
	}
	return -1;
}

RadioButton* RadioGroup::selected() const {
	const long index = selectedIndex();
	return index >= 0 ? mButtons[(size_t) index] : nullptr;
}
