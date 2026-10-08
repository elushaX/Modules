#include "TextField.hpp"

#include <algorithm>

#include "BoxLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kBlinkPeriodMs = 1060;
	constexpr halnf kCaretWidth = 1.5f;
	constexpr halnf kStepButtonWidth = 28;
	constexpr halnf kStepIconSize = 10;
	constexpr halnf kStepIconStroke = 1.6f;
}

TextField::TextField(std::string text) :
	mBuffer(TextEditBuffer::Lines::Single, std::move(text)) {
	setDebugName("text field");
	setCursor(CursorShape::Text);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void TextField::setText(std::string text) {
	if (text == mBuffer.text()) return;

	mBuffer.setText(std::move(text));
	mScroll = 0;
	requestPaint();
}

void TextField::setPlaceholder(std::string placeholder) {
	mPlaceholder = std::move(placeholder);
	requestPaint();
}

void TextField::selectAll() {
	mBuffer.selectAll();
	restartBlink();
}

void TextField::setCaret(size_t position, bool extendSelection) {
	mBuffer.setCaret(position, extendSelection);
	restartBlink();
}

Extent TextField::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	if (axis == Axis::Vertical) return Extent::rigid(metrics.controlHeight + fontSize() - metrics.fontSize);

	const halnf padding = metrics.controlPadding.left + metrics.controlPadding.right;
	const halnf sample = textWidth(text().empty() ? mPlaceholder : text(), fontSize());
	return { padding + 24, padding + sample, kUnbounded, 0 };
}

RGBA TextField::backgroundTarget() const {
	const Palette& palette = theme().palette;
	if (!isEffectivelyEnabled()) return Palette::mix(palette.field, palette.window, 0.5f);
	return palette.field;
}

void TextField::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			{
				if (event.button() != PointerButton::Primary) return;
				const size_t position = positionAt(event.position().x);
				if (event.clickCount() >= 3 || (mFocusedByPress && event.clickCount() == 1)) {
					selectAll();
				} else if (event.clickCount() == 2) {
					mBuffer.select(mBuffer.previousWord(position), mBuffer.nextWord(position));
				} else {
					setCaret(position, event.modifiers().shift);
				}
				mDragging = event.clickCount() == 1 && !mFocusedByPress;
				mFocusedByPress = false;
				if (ui()) ui()->capturePointer(*this);
				event.accept();
				break;
			}

		case Event::Type::PointerMove:
			if (!mDragging) return;
			setCaret(positionAt(event.position().x), true);
			event.accept();
			break;

		case Event::Type::PointerUp:
			if (event.button() != PointerButton::Primary) return;
			mDragging = false;
			if (ui()) ui()->releasePointer(*this);
			event.accept();
			break;

		case Event::Type::KeyDown: handleKey(event); break;

		case Event::Type::TextInput:
			insert(event.text());
			event.accept();
			break;

		default: return;
	}

	requestPaint();
}

void TextField::handleKey(Event& event) {
	const bool shift = event.modifiers().shift;
	const bool control = event.modifiers().control;

	switch (event.keyCode()) {
		case InputID::LEFT:
			if (hasSelection() && !shift) setCaret(mBuffer.selectionStart());
			else setCaret(control ? mBuffer.previousWord(caret()) : mBuffer.previousBoundary(caret()), shift);
			break;
		case InputID::RIGHT:
			if (hasSelection() && !shift) setCaret(mBuffer.selectionEnd());
			else setCaret(control ? mBuffer.nextWord(caret()) : mBuffer.nextBoundary(caret()), shift);
			break;
		case InputID::HOME: setCaret(0, shift); break;
		case InputID::END: setCaret(text().size(), shift); break;
		case InputID::BACKSPACE:
			if (mBuffer.eraseBackward(control)) edited();
			restartBlink();
			break;
		case InputID::DELETE_KEY:
			if (mBuffer.eraseForward(control)) edited();
			restartBlink();
			break;
		case InputID::ENTER:
		case InputID::KP_ENTER: commit(); break;
		case InputID::ESCAPE:
			if (ui()) ui()->setKeyboardFocus(nullptr);
			commit();
			break;
		case InputID::A:
			if (!control) return;
			selectAll();
			break;
		case InputID::C:
		case InputID::X:
			{
				if (!control) return;
				WindowHost* host = ui() ? ui()->windowHost() : nullptr;
				if (host && hasSelection()) host->setClipboardText(selectedText());
				if (event.keyCode() == InputID::X && mBuffer.eraseSelection()) edited();
				break;
			}
		case InputID::V:
			{
				if (!control) return;
				WindowHost* host = ui() ? ui()->windowHost() : nullptr;
				if (host) insert(host->clipboardText());
				break;
			}
		default:
			if (control || event.modifiers().alt) return;
			if (event.keyCode() == InputID::TAB || event.keyCode() == InputID::UP || event.keyCode() == InputID::DOWN) return;
			break;
	}

	event.accept();
}

void TextField::insert(const std::string& text) {
	if (!mBuffer.insert(text)) return;
	restartBlink();
	edited();
}

void TextField::commit() {
	if (mOnCommitted) mOnCommitted(text());
}

void TextField::edited() {
	if (mOnEdited) mOnEdited(text());
	requestPaint();
}

void TextField::updateTextInputArea() {
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (!host) return;

	const RectF area = windowRect();
	host->setTextInputArea(mFocused ? &area : nullptr);
}

void TextField::restartBlink() {
	mBlinkMs = 0;
	startTicking();
	requestPaint();
}

void TextField::onKeyboardFocusChanged() {
	const bool focused = hasKeyboardFocus();
	if (focused == mFocused) return;

	mFocused = focused;
	mFocusedByPress = focused && ui() && ui()->isPrimaryButtonDown();
	updateTextInputArea();
	restartBlink();
	if (focused) {
		if (mOnEditingStarted) mOnEditingStarted();
		return;
	}

	mBuffer.collapseSelection();
	mDragging = false;
	commit();
}

void TextField::onDetached() {
	if (!mFocused) return;
	mFocused = false;
	mDragging = false;
	updateTextInputArea();
}

void TextField::onTick(halnf deltaMs) {
	Control::onTick(deltaMs);
	if (!mFocused) return;

	const bool visibleBefore = std::fmod(mBlinkMs, kBlinkPeriodMs) < kBlinkPeriodMs / 2;
	mBlinkMs += deltaMs;
	const bool visibleAfter = std::fmod(mBlinkMs, kBlinkPeriodMs) < kBlinkPeriodMs / 2;
	if (visibleBefore != visibleAfter) requestPaint();
	startTicking();
}

halnf TextField::offsetOf(size_t position) const { return textWidth(text().substr(0, position), fontSize()); }

size_t TextField::positionAt(halnf localX) const {
	const halnf x = localX - textBox().pos.x + mScroll;
	size_t best = 0;
	halnf bestDistance = std::fabs(x);

	for (size_t position = mBuffer.nextBoundary(0); position <= text().size();
			 position = mBuffer.nextBoundary(position)) {
		const halnf distance = std::fabs(offsetOf(position) - x);
		if (distance < bestDistance) {
			bestDistance = distance;
			best = position;
		}
		if (position == text().size()) break;
	}
	return best;
}

RectF TextField::textBox() const {
	const Insets& padding = theme().metrics.controlPadding;
	return makeRect(padding.left, 0, size().x - padding.left - padding.right, size().y);
}

halnf TextField::fontSize() const { return mFontSize > 0 ? mFontSize : theme().metrics.fontSize; }

halnf TextField::lineHeight() const {
	const Metrics& metrics = theme().metrics;
	return metrics.lineHeight * fontSize() / metrics.fontSize;
}

void TextField::setFontSize(halnf fontSize) {
	mFontSize = fontSize;
	requestLayout();
}

void TextField::paint(Painter& painter) {
	const Theme& style = theme();
	const Palette& palette = style.palette;
	const RectF box = textBox();

	const halnf caretX = offsetOf(caret());
	if (mFocused) {
		if (caretX - mScroll > box.size.x) mScroll = caretX - box.size.x;
		if (caretX - mScroll < 0) mScroll = caretX;
		const halnf overflow = offsetOf(text().size()) - box.size.x;
		mScroll = std::clamp(mScroll, 0.0f, std::max(0.0f, overflow));
	} else {
		mScroll = 0;
	}

	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);
	const RGBA border = mFocused ? palette.mutedText : palette.controlBorder;
	painter.strokeRect(localRect(), border, style.metrics.cornerRadius, style.metrics.borderWidth);

	ScopedClip clip(painter, box);
	const RectF scrolled = makeRect(box.pos.x - mScroll, 0, box.size.x + mScroll, box.size.y);

	if (mFocused && hasSelection()) {
		const halnf from = offsetOf(mBuffer.selectionStart());
		const halnf to = offsetOf(mBuffer.selectionEnd());
		const halnf height = lineHeight();
		painter.fillRect(
			makeRect(scrolled.pos.x + from, (size().y - height) / 2, to - from, height), palette.neutralSelection, 2
		);
	}

	if (text().empty() && !mPlaceholder.empty()) {
		painter.text(mPlaceholder, box, fontSize(), TextAlign::Start, palette.disabledText);
	} else {
		const RGBA color = isEffectivelyEnabled() ? palette.text : palette.disabledText;
		painter.text(text(), scrolled, fontSize(), TextAlign::Start, color);
	}

	const bool caretVisible = std::fmod(mBlinkMs, kBlinkPeriodMs) < kBlinkPeriodMs / 2;
	if (mFocused && caretVisible) {
		const halnf height = lineHeight();
		painter.fillRect(
			makeRect(scrolled.pos.x + caretX - kCaretWidth / 2, (size().y - height) / 2, kCaretWidth, height), palette.text
		);
	}
}

StepButton::StepButton(int direction) :
	Button(""),
	mDirection(direction) {
	setDebugName(direction < 0 ? "decrement" : "increment");
	setTooltip(direction < 0 ? "Decrease" : "Increase");
	setIconSize(kStepIconSize);
}

void StepButton::paintIcon(Painter& painter, const RectF& box) {
	const Vec2F center = box.pos + box.size / 2.0f;
	const halnf half = std::min(box.size.x, box.size.y) / 2;
	const RGBA color = isEffectivelyEnabled() ? theme().palette.text : theme().palette.mutedText;
	painter.line(center - Vec2F(half, 0.0f), center + Vec2F(half, 0.0f), color, kStepIconStroke);
	if (mDirection > 0) painter.line(center - Vec2F(0.0f, half), center + Vec2F(0.0f, half), color, kStepIconStroke);
}

NumberField::NumberField(halnf minimum, halnf maximum, halnf value, halnf step) :
	mMinimum(minimum),
	mMaximum(maximum),
	mValue(value),
	mStep(step) {
	setDebugName("number field");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<RowLayout>(2).setCrossAlign(CrossAlign::Center);

	for (Button* button : { &mDecrement, &mIncrement })
		button->setSizing(AxisSizing::fixed(kStepButtonWidth), AxisSizing::fit());
	mDecrement.setOnActivate([this]() { stepBy(-1); });
	mIncrement.setOnActivate([this]() { stepBy(1); });

	mField.setCharacterFilter([](const std::string& character) {
		return character.size() == 1 &&
					 (std::isdigit((unsigned char) character[0]) || character == "." || character == "-" || character == ",");
	});
	mField.setOnCommitted([this](const std::string& text) {
		std::string normalized = text;
		for (char& character : normalized) {
			if (character == ',') character = '.';
		}
		char* end = nullptr;
		const double parsed = std::strtod(normalized.c_str(), &end);
		if (end != normalized.c_str()) apply((halnf) parsed);
		refreshText();
	});

	addChild(mDecrement);
	addChild(mField);
	addChild(mIncrement);

	mValue = clamped(mValue);
	refreshText();
}

void NumberField::setValue(halnf value) {
	mValue = clamped(value);
	refreshText();
}

void NumberField::setRange(halnf minimum, halnf maximum) {
	mMinimum = minimum;
	mMaximum = maximum;
	setValue(mValue);
}

void NumberField::setDecimals(int decimals) {
	mDecimals = decimals;
	refreshText();
}

void NumberField::setSuffix(std::string suffix) {
	mSuffix = std::move(suffix);
	refreshText();
}

void NumberField::stepBy(int steps) { apply(mValue + mStep * (halnf) steps); }

void NumberField::onEvent(Event& event) {
	if (event.type() == Event::Type::KeyDown) {
		if (event.keyCode() == InputID::UP || event.keyCode() == InputID::PAGE_UP) stepBy(1);
		else if (event.keyCode() == InputID::DOWN || event.keyCode() == InputID::PAGE_DOWN) stepBy(-1);
		else return;
		event.accept();
		return;
	}

	if (event.type() == Event::Type::Wheel && event.wheelAmount() != 0) {
		stepBy(event.wheelAmount() > 0 ? 1 : -1);
		event.accept();
	}
}

void NumberField::apply(halnf value) {
	const halnf next = clamped(value);
	const bool changed = next != mValue;
	mValue = next;
	refreshText();
	if (changed && mOnChanged) mOnChanged(mValue);
}

void NumberField::refreshText() { mField.setText(format(mValue)); }

halnf NumberField::clamped(halnf value) const {
	if (value < mMinimum) return mMinimum;
	if (value > mMaximum) return mMaximum;
	return value;
}

std::string NumberField::format(halnf value) const {
	char text[64];
	std::snprintf(text, sizeof(text), "%.*f", mDecimals, (double) value);
	return text + mSuffix;
}
