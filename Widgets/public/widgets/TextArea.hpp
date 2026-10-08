#pragma once

#include "Containers.hpp"
#include "Control.hpp"
#include "ResizeHandle.hpp"
#include "TextEditBuffer.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	struct TextLayoutLine {
		size_t start = 0;
		size_t end = 0;
	};

	struct HighlightSpan {
		size_t start = 0;
		size_t end = 0;
		RGBA color;
	};

	using TextHighlighter = std::function<std::vector<HighlightSpan>(const std::string& text)>;

	class TextAreaEditor : public Control {
	public:
		TextAreaEditor();

		void setText(std::string text);
		[[nodiscard]] const std::string& text() const { return mBuffer.text(); }
		void setPlaceholder(std::string placeholder);
		void setFillsWithWindowColor(bool window);
		[[nodiscard]] TextEditBuffer& buffer() { return mBuffer; }
		[[nodiscard]] const TextEditBuffer& buffer() const { return mBuffer; }

		void setCaret(size_t position, bool extendSelection = false);
		void selectAll();

		[[nodiscard]] const std::vector<TextLayoutLine>& lines() const { return mLines; }
		[[nodiscard]] size_t lineOf(size_t position) const;
		[[nodiscard]] RectF caretRect() const;
		[[nodiscard]] size_t positionAt(Vec2F localPoint) const;
		[[nodiscard]] halnf lineHeight() const;
		[[nodiscard]] bool isEditing() const { return mFocused; }

		void setOnEdited(std::function<void(const std::string&)> callback) { mOnEdited = std::move(callback); }
		void setOnCommitted(std::function<void(const std::string&)> callback) { mOnCommitted = std::move(callback); }
		void setOnCaretMoved(std::function<void()> callback) { mOnCaretMoved = std::move(callback); }
		void setOnEditingStarted(std::function<void()> callback) { mOnEditingStarted = std::move(callback); }
		void setPasteHandler(std::function<bool()> handler) { mPasteHandler = std::move(handler); }
		void insert(const std::string& text);

		void setFontFace(FontFace face);
		[[nodiscard]] FontFace fontFace() const { return mFace; }
		void setFontScale(halnf scale);
		[[nodiscard]] halnf fontScale() const { return mFontScale; }
		[[nodiscard]] halnf textTop() const;
		[[nodiscard]] halnf topOfLineContaining(size_t position) const;
		[[nodiscard]] size_t positionAtTop(halnf localY) const;
		void setHighlighter(TextHighlighter highlighter);
		[[nodiscard]] const std::vector<HighlightSpan>& highlights() const;

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;
		void onTick(halnf deltaMs) override;
		void onKeyboardFocusChanged() override;
		void onDetached() override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		struct AdvancesKey {
			halnf fontSize = 0;
			FontFace face = FontFace::Regular;
			const TextMetrics* metrics = nullptr;

			bool operator==(const AdvancesKey&) const = default;
		};

		void handlePointer(Event& event);
		void handleKey(Event& event);
		void moveVertically(int lines, bool extendSelection);
		void moveToLineEdge(bool end, bool extendSelection);
		void copySelection(bool cut);
		void paste();
		void edited();
		void caretMoved();
		void restartBlink();
		void updateTextInputArea();

		void relayoutText(halnf width) const;
		void ensureLayout() const;
		void ensureAdvances() const;
		void invalidateAdvances();
		[[nodiscard]] halnf textWidthOf(size_t from, size_t to) const;
		[[nodiscard]] halnf columnX(size_t position) const;
		[[nodiscard]] size_t positionInLine(const TextLayoutLine& line, halnf x) const;
		[[nodiscard]] halnf wrapWidth() const;
		[[nodiscard]] halnf fontSize() const;
		[[nodiscard]] TextStyle textStyle() const { return { fontSize(), mFace }; }
		void invalidateTextLayout();
		void paintLineText(Painter& painter, const TextLayoutLine& line, const RectF& box, const RGBA& baseColor) const;

		TextEditBuffer mBuffer{ TextEditBuffer::Lines::Multiple };
		FontFace mFace = FontFace::Regular;
		halnf mFontScale = 1;
		TextHighlighter mHighlighter;
		mutable std::vector<HighlightSpan> mHighlights;
		mutable bool mHighlightsDirty = true;
		std::string mPlaceholder;
		bool mWindowColored = false;
		mutable std::vector<TextLayoutLine> mLines;
		mutable std::vector<halnf> mParagraphAdvances;
		mutable std::vector<size_t> mParagraphStarts;
		mutable std::optional<AdvancesKey> mAdvancesKey;
		mutable halnf mLayoutWidth = -1;
		mutable bool mLayoutDirty = true;
		halnf mPreferredX = -1;
		halnf mBlinkMs = 0;
		bool mFocused = false;
		bool mDragging = false;
		std::function<void(const std::string&)> mOnEdited;
		std::function<void(const std::string&)> mOnCommitted;
		std::function<void()> mOnCaretMoved;
		std::function<void()> mOnEditingStarted;
		std::function<bool()> mPasteHandler;
	};

	class TextArea : public Widget {
	public:
		explicit TextArea(std::string text = {});

		void setText(std::string text) { mEditor.setText(std::move(text)); }
		[[nodiscard]] const std::string& text() const { return mEditor.text(); }
		void setPlaceholder(std::string placeholder) { mEditor.setPlaceholder(std::move(placeholder)); }

		void setOnEdited(std::function<void(const std::string&)> callback) { mEditor.setOnEdited(std::move(callback)); }
		void setOnCommitted(std::function<void(const std::string&)> callback) {
			mEditor.setOnCommitted(std::move(callback));
		}
		void setOnEditingStarted(std::function<void()> callback) { mEditor.setOnEditingStarted(std::move(callback)); }
		void setOnCaretMoved(std::function<void()> callback) { mOnCaretMoved = std::move(callback); }
		[[nodiscard]] bool isEditing() const { return mEditor.isEditing(); }

		void setBoxHeight(halnf height);
		[[nodiscard]] halnf boxHeight() const { return mBoxHeight; }
		void setHeightRange(halnf minimum, halnf maximum);
		void setFillsHeight(bool fills);
		[[nodiscard]] bool fillsHeight() const { return mFillsHeight; }
		[[nodiscard]] halnf minimumHeight() const { return mMinimumHeight; }
		[[nodiscard]] halnf maximumHeight() const { return mMaximumHeight; }

		void setFontFace(FontFace face) { mEditor.setFontFace(face); }
		void setFillsWithWindowColor(bool window);
		void setFontScale(halnf scale);
		void setHighlighter(TextHighlighter highlighter) { mEditor.setHighlighter(std::move(highlighter)); }

		[[nodiscard]] TextAreaEditor& editor() { return mEditor; }
		[[nodiscard]] const TextAreaEditor& editor() const { return mEditor; }
		[[nodiscard]] ScrollView& scroll() { return mScroll; }
		[[nodiscard]] const ScrollView& scroll() const { return mScroll; }
		[[nodiscard]] ResizeHandle& resizeHandle() { return mHandle; }

	protected:
		void paint(Painter& painter) override;
		void onTick(halnf deltaMs) override;

	private:
		class AreaLayout;

		void revealCaret();
		void applyBoxHeight();
		void viewportArranged(halnf height);
		[[nodiscard]] halnf caretRevealOffset(halnf viewportHeight) const;

		ScrollView mScroll{ Axis::Vertical };
		TextAreaEditor mEditor;
		bool mWindowColored = false;
		ResizeHandle mHandle{ Axis::Vertical };
		halnf mBoxHeight = 96;
		halnf mMinimumHeight = 48;
		halnf mMaximumHeight = 600;
		halnf mDragStartHeight = 0;
		halnf mDragStartCoordinate = 0;
		halnf mArrangedViewportHeight = -1;
		bool mFillsHeight = false;
		std::function<void()> mOnCaretMoved;
	};
}
