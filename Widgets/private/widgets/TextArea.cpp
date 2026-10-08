#include "TextArea.hpp"

#include "Event.hpp"
#include "Layout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kBlinkPeriodMs = 1060;
	constexpr halnf kCaretWidth = 1.5f;
	constexpr halnf kHandleThickness = 8;
	constexpr halnf kTextInset = 8;

	bool isSpace(char character) { return character == ' '; }
}

class TextArea::AreaLayout : public Layout {
public:
	explicit AreaLayout(TextArea& area) :
		mArea(area) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext&, Axis axis, halnf) const override {
		if (axis == Axis::Vertical && mArea.mFillsHeight)
			return Extent::shrinkable(mArea.mMinimumHeight, mArea.mMinimumHeight);
		if (axis == Axis::Vertical) return Extent::rigid(mArea.mBoxHeight);
		return { 40, 200, kUnbounded, 0 };
	}

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		context.place(mArea.mScroll, contentArea);
		const halnf top = contentArea.pos.y + contentArea.size.y - kHandleThickness;
		const halnf handleThickness = mArea.mFillsHeight ? 0 : kHandleThickness;
		context.place(mArea.mHandle, makeRect(contentArea.pos.x, top, contentArea.size.x, handleThickness));
		mArea.viewportArranged(contentArea.size.y);
	}

private:
	TextArea& mArea;
};

TextAreaEditor::TextAreaEditor() {
	setDebugName("text area editor");
	setCursor(CursorShape::Text);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void TextAreaEditor::setText(std::string text) {
	if (text == mBuffer.text()) return;

	mBuffer.setText(std::move(text));
	invalidateAdvances();
	mLayoutDirty = true;
	mHighlightsDirty = true;
	mPreferredX = -1;
	requestLayout();
	requestPaint();
}

void TextAreaEditor::setPlaceholder(std::string placeholder) {
	mPlaceholder = std::move(placeholder);
	requestPaint();
}

void TextAreaEditor::setCaret(size_t position, bool extendSelection) {
	mBuffer.setCaret(position, extendSelection);
	mPreferredX = -1;
	caretMoved();
}

void TextAreaEditor::selectAll() {
	mBuffer.selectAll();
	caretMoved();
}

halnf TextAreaEditor::fontSize() const { return theme().metrics.fontSize * mFontScale; }

halnf TextAreaEditor::lineHeight() const { return std::round(theme().metrics.lineHeight * mFontScale); }

halnf TextAreaEditor::textTop() const { return kTextInset / 2; }

halnf TextAreaEditor::topOfLineContaining(size_t position) const {
	ensureLayout();
	return textTop() + halnf(lineOf(position)) * lineHeight();
}

size_t TextAreaEditor::positionAtTop(halnf localY) const {
	ensureLayout();
	if (mLines.empty()) return 0;

	const halnf row = std::max(0.0f, localY - textTop()) / lineHeight();
	const size_t index = std::min((size_t) row, mLines.size() - 1);
	return mLines[index].start;
}

void TextAreaEditor::invalidateTextLayout() {
	mLayoutDirty = true;
	mPreferredX = -1;
	requestLayout();
	requestPaint();
}

void TextAreaEditor::setFontScale(halnf scale) {
	if (scale == mFontScale) return;

	mFontScale = scale;
	invalidateTextLayout();
}

halnf TextAreaEditor::wrapWidth() const { return std::max(1.0f, size().x - kTextInset * 2); }

void TextAreaEditor::invalidateAdvances() { mAdvancesKey.reset(); }

void TextAreaEditor::ensureAdvances() const {
	const AdvancesKey key = { fontSize(), mFace, &textMetrics() };
	if (mAdvancesKey == key) return;

	const std::string& text = mBuffer.text();
	mParagraphAdvances.assign(text.size() + 1, 0);
	mParagraphStarts.assign(text.size() + 1, 0);

	size_t start = 0;
	while (start <= text.size()) {
		const size_t end = mBuffer.lineEnd(start);
		const std::vector<halnf> widths = prefixWidths(text.substr(start, end - start), textStyle());
		for (size_t offset = 0; offset <= end - start; offset++) {
			mParagraphAdvances[start + offset] = widths[offset];
			mParagraphStarts[start + offset] = start;
		}
		start = end + 1;
	}
	mAdvancesKey = key;
}

halnf TextAreaEditor::textWidthOf(size_t from, size_t to) const {
	if (to <= from) return 0;

	ensureAdvances();
	if (mParagraphStarts[from] != mParagraphStarts[to])
		return textWidth(mBuffer.text().substr(from, to - from), textStyle());
	return mParagraphAdvances[to] - mParagraphAdvances[from];
}

void TextAreaEditor::setFontFace(FontFace face) {
	if (face == mFace) return;

	mFace = face;
	invalidateTextLayout();
}

void TextAreaEditor::setHighlighter(TextHighlighter highlighter) {
	mHighlighter = std::move(highlighter);
	mHighlightsDirty = true;
	requestPaint();
}

const std::vector<HighlightSpan>& TextAreaEditor::highlights() const {
	if (!mHighlightsDirty) return mHighlights;

	mHighlights = mHighlighter ? mHighlighter(mBuffer.text()) : std::vector<HighlightSpan>{};
	std::sort(mHighlights.begin(), mHighlights.end(), [](const HighlightSpan& a, const HighlightSpan& b) {
		return a.start < b.start;
	});
	mHighlightsDirty = false;
	return mHighlights;
}

void TextAreaEditor::paintLineText(
	Painter& painter, const TextLayoutLine& line, const RectF& box, const RGBA& baseColor
) const {
	const std::string& text = mBuffer.text();
	const std::vector<HighlightSpan>& spans = highlights();
	if (spans.empty() || !isEffectivelyEnabled()) {
		painter.styledText(text.substr(line.start, line.end - line.start), box, textStyle(), TextAlign::Start, baseColor);
		return;
	}

	size_t position = line.start;
	auto span =
		std::lower_bound(spans.begin(), spans.end(), line.start, [](const HighlightSpan& candidate, size_t start) {
			return candidate.end <= start;
		});

	const auto drawPiece = [&](size_t from, size_t to, const RGBA& color) {
		if (to <= from) return;
		const RectF piece = makeRect(box.pos.x + textWidthOf(line.start, from), box.pos.y, box.size.x, box.size.y);
		painter.styledText(text.substr(from, to - from), piece, textStyle(), TextAlign::Start, color);
	};

	while (position < line.end) {
		while (span != spans.end() && span->end <= position)
			++span;
		if (span == spans.end() || span->start >= line.end) {
			drawPiece(position, line.end, baseColor);
			return;
		}

		const size_t start = std::max(span->start, position);
		drawPiece(position, start, baseColor);
		const size_t end = std::min(span->end, line.end);
		drawPiece(start, end, span->color);
		position = end;
	}
}

void TextAreaEditor::relayoutText(halnf width) const {
	const std::string& text = mBuffer.text();
	mLines.clear();

	size_t paragraphStart = 0;
	while (true) {
		const size_t paragraphEnd = mBuffer.lineEnd(paragraphStart);
		size_t lineStart = paragraphStart;

		while (true) {
			size_t lineEnd = lineStart;
			size_t lastBreak = std::string::npos;

			while (lineEnd < paragraphEnd) {
				const size_t next = mBuffer.nextBoundary(lineEnd);
				if (textWidthOf(lineStart, next) > width && lineEnd > lineStart) break;
				if (isSpace(text[lineEnd])) lastBreak = next;
				lineEnd = next;
			}

			if (lineEnd < paragraphEnd && lastBreak != std::string::npos && lastBreak > lineStart) lineEnd = lastBreak;
			mLines.push_back({ lineStart, lineEnd });
			if (lineEnd >= paragraphEnd) break;
			lineStart = lineEnd;
		}

		if (paragraphEnd >= text.size()) break;
		paragraphStart = paragraphEnd + 1;
	}

	mLayoutWidth = width;
	mLayoutDirty = false;
}

void TextAreaEditor::ensureLayout() const {
	if (mLayoutDirty || mLayoutWidth != wrapWidth()) relayoutText(wrapWidth());
}

size_t TextAreaEditor::lineOf(size_t position) const {
	ensureLayout();
	for (size_t index = 0; index + 1 < mLines.size(); index++) {
		if (position < mLines[index + 1].start) return index;
	}
	return mLines.size() - 1;
}

halnf TextAreaEditor::columnX(size_t position) const {
	const TextLayoutLine& line = mLines[lineOf(position)];
	return kTextInset + textWidthOf(line.start, std::min(position, line.end));
}

RectF TextAreaEditor::caretRect() const {
	const size_t line = lineOf(mBuffer.caret());
	return makeRect(
		columnX(mBuffer.caret()) - kCaretWidth / 2, kTextInset / 2 + halnf(line) * lineHeight(), kCaretWidth, lineHeight()
	);
}

size_t TextAreaEditor::positionInLine(const TextLayoutLine& line, halnf x) const {
	size_t best = line.start;
	halnf bestDistance = std::fabs(x - kTextInset);

	for (size_t position = line.start; position < line.end;) {
		position = mBuffer.nextBoundary(position);
		const halnf distance = std::fabs(kTextInset + textWidthOf(line.start, position) - x);
		if (distance < bestDistance) {
			bestDistance = distance;
			best = position;
		}
	}

	const bool softWrapped = line.end < mBuffer.text().size() && mBuffer.text()[line.end] != '\n';
	if (softWrapped && best == line.end && line.end > line.start) best = mBuffer.previousBoundary(line.end);
	return best;
}

size_t TextAreaEditor::positionAt(Vec2F localPoint) const {
	ensureLayout();
	const halnf row = std::floor((localPoint.y - kTextInset / 2) / lineHeight());
	const size_t index = (size_t) std::clamp(row, 0.0f, halnf(mLines.size() - 1));
	return positionInLine(mLines[index], localPoint.x);
}

Extent TextAreaEditor::measureContent(Axis axis, halnf crossLength) const {
	if (axis == Axis::Horizontal) return { 40, 200, kUnbounded, 0 };

	const halnf width = std::max(1.0f, crossLength - kTextInset * 2);
	if (mLayoutDirty || mLayoutWidth != width) relayoutText(width);
	return Extent::rigid(halnf(mLines.size()) * lineHeight() + kTextInset);
}

RGBA TextAreaEditor::backgroundTarget() const {
	const Palette& palette = theme().palette;
	return mWindowColored ? palette.window : palette.field;
}

void TextAreaEditor::setFillsWithWindowColor(bool window) {
	mWindowColored = window;
	refreshVisualState();
}

void TextAreaEditor::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
		case Event::Type::PointerMove:
		case Event::Type::PointerUp: handlePointer(event); break;
		case Event::Type::KeyDown: handleKey(event); break;
		case Event::Type::TextInput:
			insert(event.text());
			event.accept();
			break;
		default: return;
	}
	requestPaint();
}

void TextAreaEditor::handlePointer(Event& event) {
	if (event.button() != PointerButton::Primary && event.type() != Event::Type::PointerMove) return;

	if (event.type() == Event::Type::PointerDown) {
		const size_t position = positionAt(event.position());
		if (event.clickCount() >= 3) {
			mBuffer.select(mBuffer.lineStart(position), mBuffer.lineEnd(position));
		} else if (event.clickCount() == 2) {
			mBuffer.select(mBuffer.previousWord(position), mBuffer.nextWord(position));
		} else {
			mBuffer.setCaret(position, event.modifiers().shift);
		}
		mPreferredX = -1;
		mDragging = event.clickCount() == 1;
		ui()->setKeyboardFocus(this);
		ui()->capturePointer(*this);
		caretMoved();
		event.accept();
		return;
	}

	if (event.type() == Event::Type::PointerMove) {
		if (!mDragging) return;
		mBuffer.setCaret(positionAt(event.position()), true);
		caretMoved();
		event.accept();
		return;
	}

	if (!mDragging) return;
	mDragging = false;
	ui()->releasePointer(*this);
	event.accept();
}

void TextAreaEditor::handleKey(Event& event) {
	const bool shift = event.modifiers().shift;
	const bool control = event.modifiers().control;
	const size_t caret = mBuffer.caret();

	switch (event.keyCode()) {
		case InputID::LEFT:
			if (mBuffer.hasSelection() && !shift) setCaret(mBuffer.selectionStart());
			else setCaret(control ? mBuffer.previousWord(caret) : mBuffer.previousBoundary(caret), shift);
			break;
		case InputID::RIGHT:
			if (mBuffer.hasSelection() && !shift) setCaret(mBuffer.selectionEnd());
			else setCaret(control ? mBuffer.nextWord(caret) : mBuffer.nextBoundary(caret), shift);
			break;
		case InputID::UP: moveVertically(-1, shift); break;
		case InputID::DOWN: moveVertically(1, shift); break;
		case InputID::PAGE_UP: moveVertically(-8, shift); break;
		case InputID::PAGE_DOWN: moveVertically(8, shift); break;
		case InputID::HOME:
			if (control) setCaret(0, shift);
			else moveToLineEdge(false, shift);
			break;
		case InputID::END:
			if (control) setCaret(mBuffer.text().size(), shift);
			else moveToLineEdge(true, shift);
			break;
		case InputID::BACKSPACE:
			if (mBuffer.eraseBackward(control)) edited();
			break;
		case InputID::DELETE_KEY:
			if (mBuffer.eraseForward(control)) edited();
			break;
		case InputID::ENTER:
		case InputID::KP_ENTER:
			if (control) {
				if (mOnCommitted) mOnCommitted(mBuffer.text());
				break;
			}
			insert("\n");
			break;
		case InputID::ESCAPE: ui()->setKeyboardFocus(nullptr); break;
		case InputID::A:
			if (!control) return;
			selectAll();
			break;
		case InputID::C:
		case InputID::X:
			if (!control) return;
			copySelection(event.keyCode() == InputID::X);
			break;
		case InputID::V:
			if (!control) return;
			paste();
			break;
		default:
			if (control || event.modifiers().alt || event.keyCode() == InputID::TAB) return;
			break;
	}

	event.accept();
	restartBlink();
}

void TextAreaEditor::moveVertically(int lines, bool extendSelection) {
	ensureLayout();
	const size_t current = lineOf(mBuffer.caret());
	if (mPreferredX < 0) mPreferredX = columnX(mBuffer.caret());

	const long target = std::clamp<long>(long(current) + lines, 0, long(mLines.size()) - 1);
	size_t position = positionInLine(mLines[target], mPreferredX);
	if (lines < 0 && long(current) + lines < 0) position = 0;
	if (lines > 0 && long(current) + lines >= long(mLines.size())) position = mBuffer.text().size();

	const halnf preferredX = mPreferredX;
	mBuffer.setCaret(position, extendSelection);
	mPreferredX = preferredX;
	caretMoved();
}

void TextAreaEditor::moveToLineEdge(bool end, bool extendSelection) {
	ensureLayout();
	const TextLayoutLine& line = mLines[lineOf(mBuffer.caret())];
	setCaret(end ? line.end : line.start, extendSelection);
}

void TextAreaEditor::copySelection(bool cut) {
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (!mBuffer.hasSelection()) return;

	if (host) host->setClipboardText(mBuffer.selectedText());
	if (cut && mBuffer.eraseSelection()) edited();
}

void TextAreaEditor::paste() {
	if (mPasteHandler && mPasteHandler()) return;

	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (host) insert(host->clipboardText());
}

void TextAreaEditor::insert(const std::string& text) {
	if (mBuffer.insert(text)) edited();
}

void TextAreaEditor::edited() {
	invalidateAdvances();
	mLayoutDirty = true;
	mHighlightsDirty = true;
	mPreferredX = -1;
	requestLayout();
	if (mOnEdited) mOnEdited(mBuffer.text());
	caretMoved();
}

void TextAreaEditor::caretMoved() {
	restartBlink();
	if (mOnCaretMoved) mOnCaretMoved();
}

void TextAreaEditor::restartBlink() {
	mBlinkMs = 0;
	if (mFocused) startTicking();
	requestPaint();
}

void TextAreaEditor::updateTextInputArea() {
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (!host) return;

	const RectF area = windowRect();
	host->setTextInputArea(mFocused ? &area : nullptr);
}

void TextAreaEditor::onKeyboardFocusChanged() {
	const bool focused = hasKeyboardFocus();
	if (focused == mFocused) return;

	mFocused = focused;
	updateTextInputArea();
	restartBlink();
	if (focused) {
		if (mOnEditingStarted) mOnEditingStarted();
		return;
	}

	mBuffer.collapseSelection();
	mDragging = false;
	stopTicking();
	if (mOnCommitted) mOnCommitted(mBuffer.text());
}

void TextAreaEditor::onDetached() {
	if (!mFocused) return;
	mFocused = false;
	mDragging = false;
	updateTextInputArea();
}

void TextAreaEditor::onTick(halnf deltaMs) {
	Control::onTick(deltaMs);
	if (!mFocused) return;

	const bool visibleBefore = std::fmod(mBlinkMs, kBlinkPeriodMs) < kBlinkPeriodMs / 2;
	mBlinkMs += deltaMs;
	const bool visibleAfter = std::fmod(mBlinkMs, kBlinkPeriodMs) < kBlinkPeriodMs / 2;
	if (visibleBefore != visibleAfter) requestPaint();
	startTicking();
}

void TextAreaEditor::paint(Painter& painter) {
	ensureLayout();
	const Palette& palette = theme().palette;
	const halnf height = lineHeight();
	const halnf top = kTextInset / 2;

	if (mBuffer.text().empty() && !mPlaceholder.empty()) {
		painter.text(
			mPlaceholder, makeRect(kTextInset, top, wrapWidth(), height), fontSize(), TextAlign::Start, palette.disabledText
		);
	}

	for (size_t index = 0; index < mLines.size(); index++) {
		const TextLayoutLine& line = mLines[index];
		const halnf y = top + halnf(index) * height;
		if (painter.isClippedOut(makeRect(0, y, size().x, height))) continue;

		if (mFocused && mBuffer.hasSelection()) {
			const size_t from = std::max(line.start, mBuffer.selectionStart());
			const size_t to = std::min(line.end, mBuffer.selectionEnd());
			const bool coversBreak = mBuffer.selectionEnd() > line.end && mBuffer.selectionStart() <= line.end;
			if (from < to || coversBreak) {
				const halnf x = kTextInset + textWidthOf(line.start, std::min(from, line.end));
				const halnf width =
					textWidthOf(std::min(from, line.end), std::max(from, to)) + (coversBreak ? fontSize() / 3 : 0);
				painter.fillRect(makeRect(x, y, width, height), palette.neutralSelection, 2);
			}
		}

		const RGBA color = isEffectivelyEnabled() ? palette.text : palette.disabledText;
		paintLineText(painter, line, makeRect(kTextInset, y, wrapWidth() + kTextInset, height), color);
	}

	const bool caretVisible = std::fmod(mBlinkMs, kBlinkPeriodMs) < kBlinkPeriodMs / 2;
	if (mFocused && caretVisible) painter.fillRect(caretRect(), palette.text);
}

TextArea::TextArea(std::string text) {
	setDebugName("text area");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	setClipsChildren(true);
	setLayout(std::make_unique<AreaLayout>(*this));

	mEditor.setText(std::move(text));
	mEditor.setOnCaretMoved([this] {
		revealCaret();
		if (mOnCaretMoved) mOnCaretMoved();
	});
	mScroll.setContent(&mEditor);
	mScroll.setWheelStep(theme().metrics.lineHeight * 3);

	mHandle.setCursor(CursorShape::ResizeVertical);
	mHandle.setOnDragStarted([this] {
		mDragStartHeight = mBoxHeight;
		mDragStartCoordinate = mHandle.windowRect().center().y;
	});
	mHandle.setOnDragged([this](halnf coordinate) {
		setBoxHeight(mDragStartHeight + coordinate - mDragStartCoordinate);
	});

	addChild(mScroll);
	addChild(mHandle);
	applyBoxHeight();
}

void TextArea::setHeightRange(halnf minimum, halnf maximum) {
	mMinimumHeight = minimum;
	mMaximumHeight = std::max(minimum, maximum);
	setBoxHeight(mBoxHeight);
}

void TextArea::setBoxHeight(halnf height) {
	mBoxHeight = std::clamp(height, mMinimumHeight, mMaximumHeight);
	applyBoxHeight();
}

void TextArea::applyBoxHeight() { requestLayout(); }

void TextArea::setFontScale(halnf scale) {
	mEditor.setFontScale(scale);
	mScroll.setWheelStep(mEditor.lineHeight() * 3);
}

void TextArea::setFillsHeight(bool fills) {
	if (fills == mFillsHeight) return;

	mFillsHeight = fills;
	mHandle.setVisible(!fills);
	setSizing(AxisSizing::fill(), fills ? AxisSizing::fill() : AxisSizing::fit());
	requestLayout();
}

void TextArea::viewportArranged(halnf height) {
	if (height == mArrangedViewportHeight) return;

	mArrangedViewportHeight = height;
	if (mEditor.isEditing()) startTicking();
}

void TextArea::onTick(halnf) {
	stopTicking();
	revealCaret();
}

halnf TextArea::caretRevealOffset(halnf viewportHeight) const {
	const RectF caret = mEditor.caretRect();
	const halnf top = caret.pos.y - kTextInset / 2;
	const halnf bottom = caret.pos.y + caret.size.y + kTextInset / 2;
	const halnf offset = mScroll.offset();

	if (top < offset) return top;
	if (bottom > offset + viewportHeight) return bottom - viewportHeight;
	return offset;
}

void TextArea::revealCaret() {
	if (ui()) ui()->updateLayout();
	if (!mScroll.size().y) return;

	mScroll.jumpTo(caretRevealOffset(mScroll.size().y));
}

void TextArea::setFillsWithWindowColor(bool window) {
	mWindowColored = window;
	mEditor.setFillsWithWindowColor(window);
	requestPaint();
}

void TextArea::paint(Painter& painter) {
	const Theme& style = theme();
	const RGBA fill = mWindowColored ? style.palette.window : style.palette.field;
	painter.fillRect(localRect(), fill, style.metrics.cornerRadius);
}
