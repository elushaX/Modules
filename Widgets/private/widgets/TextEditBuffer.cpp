#include "TextEditBuffer.hpp"

#include <algorithm>
#include <cctype>

using namespace tp;
using namespace tp::ui;

namespace {
	bool isContinuationByte(char byte) { return ((unsigned char) byte & 0xC0) == 0x80; }

	bool isWordCharacter(char byte) {
		const auto value = (unsigned char) byte;
		return std::isalnum(value) || value == '_' || value >= 0x80;
	}

	size_t characterLength(const std::string& text, size_t index) {
		size_t length = 1;
		while (index + length < text.size() && isContinuationByte(text[index + length]))
			length++;
		return length;
	}

	size_t cutAtBoundary(const std::string& text, size_t length) {
		while (length > 0 && length < text.size() && isContinuationByte(text[length]))
			length--;
		return length;
	}
}

TextEditBuffer::TextEditBuffer(Lines lines, std::string text) :
	mLines(lines) {
	setText(std::move(text));
}

void TextEditBuffer::setText(std::string text) {
	mText = std::move(text);
	if (mMaximumLength && mText.size() > mMaximumLength) mText.resize(cutAtBoundary(mText, mMaximumLength));
	mCaret = mAnchor = mText.size();
}

void TextEditBuffer::setMaximumLength(size_t maximumLength) {
	mMaximumLength = maximumLength;
	if (!mMaximumLength || mText.size() <= mMaximumLength) return;

	mText.resize(cutAtBoundary(mText, mMaximumLength));
	mCaret = std::min(mCaret, mText.size());
	mAnchor = std::min(mAnchor, mText.size());
}

std::string TextEditBuffer::selectedText() const {
	return mText.substr(selectionStart(), selectionEnd() - selectionStart());
}

size_t TextEditBuffer::clampedToBoundary(size_t position) const {
	position = std::min(position, mText.size());
	while (position > 0 && position < mText.size() && isContinuationByte(mText[position]))
		position--;
	return position;
}

void TextEditBuffer::setCaret(size_t position, bool extendSelection) {
	mCaret = clampedToBoundary(position);
	if (!extendSelection) mAnchor = mCaret;
}

void TextEditBuffer::select(size_t anchor, size_t caret) {
	mAnchor = clampedToBoundary(anchor);
	mCaret = clampedToBoundary(caret);
}

void TextEditBuffer::selectAll() { select(0, mText.size()); }

bool TextEditBuffer::accepts(const std::string& character) const {
	const auto lead = (unsigned char) character[0];
	const bool newline = character == "\n";

	if (newline && mLines == Lines::Single) return false;
	if (lead < 0x20 && !newline) return false;
	return !mAccepts || mAccepts(character);
}

std::string TextEditBuffer::acceptedCharacters(const std::string& text) const {
	std::string accepted;
	for (size_t index = 0; index < text.size();) {
		const size_t length = characterLength(text, index);
		std::string character = text.substr(index, length);
		index += length;

		if (character == "\r") continue;
		if (character == "\t") character = " ";
		if (accepts(character)) accepted += character;
	}
	return accepted;
}

bool TextEditBuffer::insert(const std::string& text) {
	std::string accepted = acceptedCharacters(text);
	if (accepted.empty()) return false;

	const size_t remaining = mText.size() - (selectionEnd() - selectionStart());
	if (mMaximumLength && remaining + accepted.size() > mMaximumLength) {
		const size_t room = mMaximumLength > remaining ? mMaximumLength - remaining : 0;
		accepted.resize(cutAtBoundary(accepted, std::min(room, accepted.size())));
		if (accepted.empty()) return false;
	}

	eraseSelection();
	mText.insert(mCaret, accepted);
	setCaret(mCaret + accepted.size());
	return true;
}

bool TextEditBuffer::eraseSelection() {
	if (!hasSelection()) return false;
	return erase(selectionStart(), selectionEnd());
}

bool TextEditBuffer::erase(size_t from, size_t to) {
	from = clampedToBoundary(from);
	to = clampedToBoundary(to);
	if (to <= from) return false;

	mText.erase(from, to - from);
	setCaret(from);
	return true;
}

bool TextEditBuffer::eraseBackward(bool wholeWord) {
	if (hasSelection()) return eraseSelection();
	return erase(wholeWord ? previousWord(mCaret) : previousBoundary(mCaret), mCaret);
}

bool TextEditBuffer::eraseForward(bool wholeWord) {
	if (hasSelection()) return eraseSelection();
	return erase(mCaret, wholeWord ? nextWord(mCaret) : nextBoundary(mCaret));
}

size_t TextEditBuffer::previousBoundary(size_t position) const {
	if (position == 0) return 0;
	position--;
	while (position > 0 && isContinuationByte(mText[position]))
		position--;
	return position;
}

size_t TextEditBuffer::nextBoundary(size_t position) const {
	if (position >= mText.size()) return mText.size();
	position++;
	while (position < mText.size() && isContinuationByte(mText[position]))
		position++;
	return position;
}

size_t TextEditBuffer::previousWord(size_t position) const {
	while (position > 0 && !isWordCharacter(mText[position - 1]))
		position--;
	while (position > 0 && isWordCharacter(mText[position - 1]))
		position--;
	return position;
}

size_t TextEditBuffer::nextWord(size_t position) const {
	while (position < mText.size() && !isWordCharacter(mText[position]))
		position++;
	while (position < mText.size() && isWordCharacter(mText[position]))
		position++;
	return position;
}

size_t TextEditBuffer::lineStart(size_t position) const {
	position = std::min(position, mText.size());
	const size_t newline = position == 0 ? std::string::npos : mText.rfind('\n', position - 1);
	return newline == std::string::npos ? 0 : newline + 1;
}

size_t TextEditBuffer::lineEnd(size_t position) const {
	const size_t newline = mText.find('\n', std::min(position, mText.size()));
	return newline == std::string::npos ? mText.size() : newline;
}
