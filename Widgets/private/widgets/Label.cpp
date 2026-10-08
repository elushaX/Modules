#include "Label.hpp"

#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	std::vector<std::string> splitLines(const std::string& text) {
		std::vector<std::string> lines;
		size_t start = 0;
		while (true) {
			const size_t end = text.find('\n', start);
			lines.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
			if (end == std::string::npos) return lines;
			start = end + 1;
		}
	}

	bool isBlank(const std::string& line) { return line.find_first_not_of(" \t") == std::string::npos; }

	bool isContinuationByte(char byte) { return (static_cast<unsigned char>(byte) & 0xC0) == 0x80; }

	void removeLastCharacter(std::string& text) {
		while (!text.empty() && isContinuationByte(text.back()))
			text.pop_back();
		if (!text.empty()) text.pop_back();
	}
}

std::vector<std::string> tp::ui::wrapText(
	const std::string& text,
	halnf width,
	const std::function<halnf(const std::string&)>& measure,
	std::optional<size_t> maximumLines
) {
	std::vector<std::string> lines;
	const auto isFull = [&] { return maximumLines && lines.size() > *maximumLines; };

	for (const std::string& paragraph : splitLines(text)) {
		std::string line;
		size_t position = 0;
		while (position <= paragraph.size() && !isFull()) {
			const size_t space = paragraph.find(' ', position);
			const size_t end = space == std::string::npos ? paragraph.size() : space;
			std::string word = paragraph.substr(position, end - position);
			position = end + 1;
			const std::string candidate = line.empty() ? word : line + " " + word;
			const halnf candidateWidth = measure(candidate);
			if (line.empty() || candidateWidth <= width) {
				line = candidate;
			} else {
				lines.push_back(line);
				line = word;
			}
			while (line.size() > 1 && (line == candidate ? candidateWidth : measure(line)) > width && !isFull()) {
				size_t cut = line.size() - 1;
				while (cut > 1 && measure(line.substr(0, cut)) > width)
					cut--;
				lines.push_back(line.substr(0, cut));
				line = line.substr(cut);
			}
			if (space == std::string::npos) break;
		}
		if (isFull()) return lines;
		lines.push_back(line);
	}
	return lines;
}

Label::Label(std::string text) :
	mText(std::move(text)),
	mLines(splitLines(mText)) {
	setDebugName("label");
}

void Label::setText(std::string text) {
	if (text == mText) return;

	mText = std::move(text);
	mLines = splitLines(mText);
	invalidateLines();
	requestLayout();
}

void Label::setAlignment(TextAlign alignment) {
	mAlignment = alignment;
	requestPaint();
}

void Label::setFontSize(halnf fontSize) {
	mFontSize = fontSize;
	requestLayout();
}

void Label::setWrap(bool wrap) {
	if (wrap == mWrap) return;
	mWrap = wrap;
	invalidateLines();
	requestLayout();
}

void Label::setMaximumLines(std::optional<size_t> lines) {
	if (lines == mMaximumLines) return;
	mMaximumLines = lines;
	invalidateLines();
	requestLayout();
}

void Label::setElidesToHeight(bool elides) {
	if (elides == mElidesToHeight) return;
	mElidesToHeight = elides;
	requestLayout();
}

void Label::invalidateLines() {
	mLaidOut.key.reset();
	mWidestLine.key.reset();
}

Label::TextLayoutKey Label::layoutKey(halnf width) const {
	const TextMetrics* metrics = ui() ? &ui()->textMetrics() : &Ui::fallbackTextMetrics();
	return { width, effectiveFontSize(), metrics };
}

const std::vector<std::string>& Label::linesFor(halnf width) const {
	const TextLayoutKey key = layoutKey(width);
	if (mLaidOut.key != key) {
		mLaidOut.lines = layOutLines(width);
		mLaidOut.key = key;
	}
	return mLaidOut.lines;
}

std::vector<std::string> Label::layOutLines(halnf width) const {
	if (!mWrap || width >= kUnbounded || width <= 0) return limitedLines(mLines, width);

	const halnf fontSize = effectiveFontSize();
	const auto measure = [this, fontSize](const std::string& line) { return textWidth(line, fontSize); };
	return limitedLines(wrapText(mText, width, measure, mMaximumLines), width);
}

std::vector<std::string> Label::limitedLines(std::vector<std::string> lines, halnf width) const {
	if (!mMaximumLines || lines.size() <= *mMaximumLines) return lines;
	return elidedLines(std::move(lines), *mMaximumLines, width);
}

std::vector<std::string> Label::elidedLines(std::vector<std::string> lines, size_t count, halnf width) const {
	lines.resize(count);
	while (lines.size() > 1 && isBlank(lines.back()))
		lines.pop_back();
	if (lines.empty()) return lines;

	const std::string ellipsis = "\u2026";
	std::string& last = lines.back();
	while (!last.empty() && width < kUnbounded && textWidth(last + ellipsis, effectiveFontSize()) > width)
		removeLastCharacter(last);
	while (!last.empty() && last.back() == ' ')
		last.pop_back();
	last += ellipsis;
	return lines;
}

size_t Label::linesFittingHeight() const {
	const halnf height = lineHeight();
	if (height <= 0) return 1;
	return std::max<size_t>(1, size_t(std::floor(size().y / height + 0.01f)));
}

std::vector<std::string> Label::visibleLines() const {
	const std::vector<std::string>& lines = linesFor(size().x);
	if (!mElidesToHeight || lines.size() <= linesFittingHeight()) return lines;
	return elidedLines(lines, linesFittingHeight(), size().x);
}

void Label::setTone(Tone tone) {
	mTone = tone;
	requestPaint();
}

void Label::setReservedText(std::string text) {
	if (mReservedText == text) return;
	mReservedText = std::move(text);
	invalidateLines();
	requestLayout();
}

Extent Label::measureContent(Axis axis, halnf crossLength) const {
	if (axis == Axis::Vertical) {
		const halnf natural = lineHeight() * (halnf) linesFor(crossLength).size();
		if (mElidesToHeight) return Extent::shrinkable(natural, std::min(natural, lineHeight()));
		return Extent::natural(natural);
	}

	const halnf widest = widestLine();
	if (mWrap) return Extent::shrinkable(widest, std::min(widest, effectiveFontSize() * 4));
	return Extent::natural(widest);
}

halnf Label::widestLine() const {
	const TextLayoutKey key = layoutKey(kUnbounded);
	if (mWidestLine.key == key) return mWidestLine.width;

	halnf widest = mReservedText.empty() ? 0 : textWidth(mReservedText, effectiveFontSize());
	for (const std::string& line : mLines)
		widest = std::max(widest, textWidth(line, effectiveFontSize()));

	mWidestLine = { key, widest };
	return widest;
}

void Label::paint(Painter& painter) {
	const Palette& palette = theme().palette;

	RGBA color = palette.text;
	if (mTone == Tone::Muted) color = palette.mutedText;
	if (mTone == Tone::Warning) color = palette.warningText;
	if (!isEffectivelyEnabled()) color = palette.disabledText;

	const std::vector<std::string>& laidOut = linesFor(size().x);
	if (mElidesToHeight && laidOut.size() > linesFittingHeight()) paintLines(painter, visibleLines(), color);
	else paintLines(painter, laidOut, color);
}

void Label::paintLines(Painter& painter, const std::vector<std::string>& lines, const RGBA& color) const {
	if (lines.size() == 1) {
		painter.text(lines.front(), localRect(), effectiveFontSize(), mAlignment, color);
		return;
	}

	const halnf height = lineHeight();
	for (size_t index = 0; index < lines.size(); index++) {
		const RectF line = makeRect(0, height * (halnf) index, size().x, height);
		painter.text(lines[index], line, effectiveFontSize(), mAlignment, color);
	}
}

halnf Label::lineHeight() const {
	const Metrics& metrics = theme().metrics;
	return metrics.lineHeight * effectiveFontSize() / metrics.fontSize;
}

halnf Label::effectiveFontSize() const { return mFontSize > 0 ? mFontSize : theme().metrics.fontSize; }

TooltipBubble::TooltipBubble() { setDebugName("tooltip"); }

void TooltipBubble::setText(std::string text) {
	mText = std::move(text);
	requestLayout();
}

Extent TooltipBubble::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	const Insets& padding = metrics.tooltipPadding;

	if (axis == Axis::Horizontal) {
		return Extent::rigid(textWidth(mText, metrics.smallFontSize) + padding.total(Axis::Horizontal));
	}
	return Extent::rigid(metrics.lineHeight + padding.total(Axis::Vertical));
}

void TooltipBubble::paint(Painter& painter) {
	const Theme& style = theme();

	painter.fillRect(localRect(), style.palette.tooltip, style.metrics.popupCornerRadius);
	painter.strokeRect(localRect(), style.palette.controlBorder, style.metrics.popupCornerRadius);
	painter.text(
		mText,
		style.metrics.tooltipPadding.shrink(localRect()),
		style.metrics.smallFontSize,
		TextAlign::Center,
		style.palette.text
	);
}
