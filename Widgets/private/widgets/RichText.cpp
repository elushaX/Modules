#include "RichText.hpp"

#include "Event.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kCodePadding = 3;
	constexpr halnf kUnderlineOffset = 1.5f;

	struct Word {
		std::string text;
		bool endsWithSpace = false;
	};

	std::vector<Word> splitWords(const std::string& text) {
		std::vector<Word> words;
		std::string current;
		for (char character : text) {
			current += character;
			if (character != ' ') continue;
			words.push_back({ current, true });
			current.clear();
		}
		if (!current.empty()) words.push_back({ current, false });
		return words;
	}

	size_t utf8Length(const std::string& text, size_t position) {
		const unsigned char lead = (unsigned char) text[position];
		size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
		return std::min(length, text.size() - position);
	}

	std::string trimTrailingSpace(const std::string& text) {
		size_t end = text.size();
		while (end > 0 && text[end - 1] == ' ')
			end--;
		return text.substr(0, end);
	}

	class LineBuilder {
	public:
		LineBuilder(std::vector<PlacedSpanPiece>& pieces, halnf width, halnf lineHeight) :
			mPieces(pieces),
			mWidth(width),
			mLineHeight(lineHeight) {}

		[[nodiscard]] halnf remaining() const { return mWidth - mX; }
		[[nodiscard]] bool atLineStart() const { return mX <= 0; }
		[[nodiscard]] size_t lineCount() const { return mLine + 1; }

		void place(size_t span, const std::string& text, halnf width, halnf inkWidth) {
			const halnf top = (halnf) mLine * mLineHeight;
			if (!mPieces.empty() && mPieces.back().span == span && mPieces.back().box.pos.y == top && mX > 0) {
				PlacedSpanPiece& last = mPieces.back();
				last.text += text;
				last.inkWidth = last.box.size.x + inkWidth;
				last.box.size.x += width;
			} else {
				mPieces.push_back({ span, text, makeRect(mX, top, width, mLineHeight), inkWidth });
			}
			mX += width;
		}

		void newLine() {
			mLine++;
			mX = 0;
		}

	private:
		std::vector<PlacedSpanPiece>& mPieces;
		halnf mWidth;
		halnf mLineHeight;
		halnf mX = 0;
		size_t mLine = 0;
	};
}

RichText::RichText() {
	setDebugName("rich text");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void RichText::setSpans(std::vector<TextSpan> spans) {
	mSpans = std::move(spans);
	mHoveredSpan.reset();
	invalidateMeasurements();
	requestLayout();
	requestPaint();
}

void RichText::setFontSize(halnf fontSize) {
	mFontSize = fontSize;
	invalidateMeasurements();
	requestLayout();
}

halnf RichText::fontSize() const { return mFontSize > 0 ? mFontSize : theme().metrics.fontSize; }

void RichText::setLineSpacing(halnf factor) {
	mLineSpacing = factor;
	mLaidOutWidth = -1;
	requestLayout();
}

void RichText::setBaseColor(std::optional<RGBA> color) {
	mBaseColor = color;
	requestPaint();
}

void RichText::setPreservesWhitespace(bool preserves) {
	mPreservesWhitespace = preserves;
	mLaidOutWidth = -1;
	requestLayout();
}

void RichText::setAlignment(TextAlign alignment) {
	mAlignment = alignment;
	mLaidOutWidth = -1;
	requestPaint();
}

std::string RichText::plainText() const {
	std::string text;
	for (const TextSpan& span : mSpans)
		text += span.lineBreak ? std::string("\n") : span.text;
	return text;
}

halnf RichText::lineHeight() const {
	const Metrics& metrics = theme().metrics;
	return std::round(metrics.lineHeight * fontSize() / metrics.fontSize * mLineSpacing);
}

TextStyle RichText::styleOf(const TextSpan& span) const {
	const halnf size = span.face == FontFace::Monospace ? fontSize() * 0.92f : fontSize();
	return { size, span.face };
}

halnf RichText::widthOf(const std::string& text, const TextSpan& span) const {
	const halnf padding = span.background ? kCodePadding * 2 : 0;
	return textWidth(text, styleOf(span)) + padding;
}

void RichText::invalidateMeasurements() {
	mWordsKey.reset();
	mLaidOutWidth = -1;
}

RichText::MeasureKey RichText::measureKey() const { return { fontSize(), &textMetrics() }; }

const std::vector<std::vector<RichText::MeasuredWord>>& RichText::measuredWords() const {
	const MeasureKey key = measureKey();
	if (mWordsKey == key) return mWords;

	mWords.assign(mSpans.size(), {});
	for (size_t index = 0; index < mSpans.size(); index++) {
		const TextSpan& span = mSpans[index];
		if (span.lineBreak) continue;
		for (Word& word : splitWords(span.text)) {
			const halnf visibleWidth = widthOf(trimTrailingSpace(word.text), span);
			const halnf width = word.endsWithSpace ? widthOf(word.text, span) : visibleWidth;
			const halnf inkWidth = inkWidthOf(word.text, span);
			mWords[index].push_back({ std::move(word.text), width, visibleWidth, inkWidth });
		}
	}
	mWordsKey = key;
	return mWords;
}

halnf RichText::inkWidthOf(const std::string& text, const TextSpan& span) const {
	return textWidth(trimTrailingSpace(text), styleOf(span));
}

halnf RichText::widestUnbreakable() const {
	halnf widest = 0;
	for (const std::vector<MeasuredWord>& words : measuredWords()) {
		for (const MeasuredWord& word : words)
			widest = std::max(widest, word.visibleWidth);
	}
	return widest;
}

const std::vector<PlacedSpanPiece>& RichText::piecesFor(halnf width) const {
	if (width != mLaidOutWidth || mPiecesKey != measureKey()) layoutFor(width);
	return mPieces;
}

void RichText::layoutFor(halnf width) const {
	const std::vector<std::vector<MeasuredWord>>& words = measuredWords();
	mPieces.clear();
	mLaidOutWidth = width;
	mPiecesKey = measureKey();
	const halnf available = width > 0 && width < kUnbounded ? width : kUnbounded;
	LineBuilder line(mPieces, available, lineHeight());

	for (size_t index = 0; index < mSpans.size(); index++) {
		const TextSpan& span = mSpans[index];
		if (span.lineBreak) {
			line.newLine();
			continue;
		}

		for (const MeasuredWord& word : words[index]) {
			if (word.visibleWidth > line.remaining() && !line.atLineStart()) line.newLine();
			if (line.atLineStart() && word.text == " " && !mPreservesWhitespace) continue;
			if (word.visibleWidth <= line.remaining() || !line.atLineStart()) {
				line.place(index, word.text, word.width, word.inkWidth);
				continue;
			}

			std::string rest = word.text;
			while (widthOf(trimTrailingSpace(rest), span) > line.remaining() && line.atLineStart() && rest.size() > 1) {
				size_t cut = 0;
				while (cut < rest.size()) {
					const size_t next = cut + utf8Length(rest, cut);
					if (widthOf(rest.substr(0, next), span) > line.remaining() && cut > 0) break;
					cut = next;
				}
				if (cut >= rest.size()) break;
				const std::string piece = rest.substr(0, cut);
				line.place(index, piece, widthOf(piece, span), inkWidthOf(piece, span));
				line.newLine();
				rest = rest.substr(cut);
			}
			line.place(index, rest, widthOf(rest, span), inkWidthOf(rest, span));
		}
	}

	mLineCount = mSpans.empty() ? 0 : line.lineCount();
	alignLines();
}

void RichText::alignLines() const {
	if (mAlignment == TextAlign::Start || mLaidOutWidth <= 0 || mLaidOutWidth >= kUnbounded) return;

	size_t first = 0;
	while (first < mPieces.size()) {
		size_t last = first;
		while (last + 1 < mPieces.size() && mPieces[last + 1].box.pos.y == mPieces[first].box.pos.y)
			last++;

		const PlacedSpanPiece& end = mPieces[last];
		const halnf padding = mSpans[end.span].background ? kCodePadding * 2 : 0;
		const halnf used = end.box.pos.x + end.inkWidth + padding;
		const halnf slack = std::max(0.0f, mLaidOutWidth - used);
		const halnf shift = mAlignment == TextAlign::Center ? std::round(slack / 2) : slack;
		for (size_t index = first; index <= last; index++)
			mPieces[index].box.pos.x += shift;
		first = last + 1;
	}
}

Extent RichText::measureContent(Axis axis, halnf crossLength) const {
	if (axis == Axis::Vertical) {
		piecesFor(crossLength);
		return Extent::natural(lineHeight() * (halnf) mLineCount);
	}

	const std::vector<std::vector<MeasuredWord>>& words = measuredWords();
	halnf preferred = 0;
	halnf lineWidth = 0;
	for (size_t index = 0; index < mSpans.size(); index++) {
		if (mSpans[index].lineBreak) {
			preferred = std::max(preferred, lineWidth);
			lineWidth = 0;
			continue;
		}
		for (const MeasuredWord& word : words[index])
			lineWidth += word.width;
	}
	preferred = std::max(preferred, lineWidth);
	return Extent::shrinkable(preferred, std::min(preferred, widestUnbreakable()));
}

void RichText::paint(Painter& painter) {
	for (const PlacedSpanPiece& piece : piecesFor(size().x))
		paintPiece(painter, piece);
}

void RichText::paintPiece(Painter& painter, const PlacedSpanPiece& piece) const {
	const TextSpan& span = mSpans[piece.span];
	const Palette& palette = theme().palette;
	const bool isLink = !span.link.empty();
	const bool hovered = mHoveredSpan && *mHoveredSpan == piece.span;

	RGBA color = span.color.value_or(mBaseColor.value_or(palette.text));
	if (isLink) color = hovered ? palette.accentHovered : Palette::mix(palette.accent, palette.text, 0.25f);
	if (!isEffectivelyEnabled()) color = palette.disabledText;

	RectF textBox = piece.box;
	const halnf inkWidth = piece.inkWidth;
	if (span.background && inkWidth > 0) {
		const RectF chip = makeRect(textBox.pos.x, textBox.pos.y + 1, inkWidth + kCodePadding * 2, textBox.size.y - 2);
		painter.fillRect(chip, *span.background, theme().metrics.indicatorCornerRadius);
		textBox.pos.x += kCodePadding;
		textBox.size.x -= kCodePadding;
	}

	painter.styledText(piece.text, textBox, styleOf(span), TextAlign::Start, color);

	const halnf baseline = textBox.pos.y + textBox.size.y / 2 + fontSize() * 0.45f + kUnderlineOffset;
	if (span.underline || (isLink && hovered)) {
		painter.line(Vec2F(textBox.pos.x, baseline), Vec2F(textBox.pos.x + inkWidth, baseline), color, 1);
	}
	if (span.strikethrough) {
		const halnf middle = textBox.pos.y + textBox.size.y / 2;
		painter.line(Vec2F(textBox.pos.x, middle), Vec2F(textBox.pos.x + inkWidth, middle), color, 1);
	}
}

std::optional<size_t> RichText::spanAt(Vec2F localPoint) const {
	for (const PlacedSpanPiece& piece : piecesFor(size().x)) {
		if (piece.box.isInside(localPoint)) return piece.span;
	}
	return std::nullopt;
}

void RichText::updateHoverFrom(Vec2F localPoint) {
	std::optional<size_t> hovered = spanAt(localPoint);
	if (hovered && mSpans[*hovered].link.empty()) hovered.reset();
	if (hovered == mHoveredSpan) return;

	mHoveredSpan = hovered;
	setCursor(hovered ? CursorShape::Pointer : CursorShape::Default);
	setTooltip(hovered ? mSpans[*hovered].link : std::string());
	requestPaint();
}

void RichText::onHoverChanged() {
	if (isHovered()) return;
	mHoveredSpan.reset();
	setCursor(CursorShape::Default);
	requestPaint();
}

void RichText::onEvent(Event& event) {
	if (event.type() == Event::Type::PointerMove) {
		updateHoverFrom(event.position());
		return;
	}
	if (event.type() != Event::Type::Click) return;

	const std::optional<size_t> span = spanAt(event.position());
	if (!span) return;

	const TextSpan& clicked = mSpans[*span];
	if (!clicked.link.empty() && mOnLink) {
		mOnLink(clicked.link);
		event.accept();
		return;
	}
	if (mOnSource) {
		mOnSource(clicked.sourceOffset);
		event.accept();
	}
}
