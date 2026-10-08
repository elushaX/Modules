#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace tp::ui {

	class TextEditBuffer {
	public:
		enum class Lines { Single, Multiple };

		explicit TextEditBuffer(Lines lines = Lines::Single, std::string text = {});

		void setText(std::string text);
		[[nodiscard]] const std::string& text() const { return mText; }

		void setMaximumLength(size_t maximumLength);
		void setCharacterFilter(std::function<bool(const std::string&)> accepts) { mAccepts = std::move(accepts); }

		[[nodiscard]] size_t caret() const { return mCaret; }
		[[nodiscard]] size_t anchor() const { return mAnchor; }
		[[nodiscard]] size_t selectionStart() const { return mAnchor < mCaret ? mAnchor : mCaret; }
		[[nodiscard]] size_t selectionEnd() const { return mAnchor < mCaret ? mCaret : mAnchor; }
		[[nodiscard]] bool hasSelection() const { return mAnchor != mCaret; }
		[[nodiscard]] std::string selectedText() const;

		void setCaret(size_t position, bool extendSelection = false);
		void select(size_t anchor, size_t caret);
		void selectAll();
		void collapseSelection() { mAnchor = mCaret; }

		bool insert(const std::string& text);
		bool eraseSelection();
		bool erase(size_t from, size_t to);
		bool eraseBackward(bool wholeWord);
		bool eraseForward(bool wholeWord);

		[[nodiscard]] size_t previousBoundary(size_t position) const;
		[[nodiscard]] size_t nextBoundary(size_t position) const;
		[[nodiscard]] size_t previousWord(size_t position) const;
		[[nodiscard]] size_t nextWord(size_t position) const;
		[[nodiscard]] size_t lineStart(size_t position) const;
		[[nodiscard]] size_t lineEnd(size_t position) const;

	private:
		[[nodiscard]] std::string acceptedCharacters(const std::string& text) const;
		[[nodiscard]] bool accepts(const std::string& character) const;
		[[nodiscard]] size_t clampedToBoundary(size_t position) const;

		Lines mLines;
		std::string mText;
		size_t mCaret = 0;
		size_t mAnchor = 0;
		size_t mMaximumLength = 0;
		std::function<bool(const std::string&)> mAccepts;
	};
}
