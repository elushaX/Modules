#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui::markdown {

	struct SourceRange {
		size_t begin = 0;
		size_t end = 0;

		[[nodiscard]] bool isEmpty() const { return end <= begin; }
		[[nodiscard]] bool contains(size_t offset) const { return offset >= begin && offset < end; }
		void include(size_t from, size_t to);
	};

	struct InlineStyle {
		bool strong = false;
		bool emphasis = false;
		bool code = false;
		bool strikethrough = false;
		bool underline = false;
		bool highlight = false;

		[[nodiscard]] bool operator==(const InlineStyle& other) const = default;
	};

	struct TextRun {
		std::string text;
		InlineStyle style;
		std::string link;
		SourceRange source;
		bool hardBreak = false;
	};

	enum class BlockKind {
		Paragraph,
		Heading,
		Quote,
		Admonition,
		BulletList,
		OrderedList,
		ListItem,
		Code,
		Rule,
		Table,
		TableRow,
		TableCell,
		Html,
		Image
	};

	enum class CellAlign { Default, Left, Center, Right };

	struct Block {
		BlockKind kind = BlockKind::Paragraph;
		SourceRange source;
		std::vector<TextRun> runs;
		std::vector<Block> children;

		int headingLevel = 0;
		unsigned listStart = 1;
		bool tightList = true;
		std::optional<bool> taskChecked;
		size_t taskMarkOffset = 0;
		std::string codeLanguage;
		std::string code;
		bool headerRow = false;
		CellAlign align = CellAlign::Default;
		std::string admonitionType;
		std::string imageSource;
		std::string imageAlt;

		[[nodiscard]] std::string plainText() const;
	};

	struct HeadingEntry {
		int level = 0;
		std::string title;
		std::string anchor;
		size_t sourceOffset = 0;
	};

	class MarkdownDocument {
	public:
		MarkdownDocument() = default;
		explicit MarkdownDocument(std::string source);

		void setSource(std::string source);
		[[nodiscard]] const std::string& source() const { return mSource; }
		[[nodiscard]] const std::vector<Block>& blocks() const { return mBlocks; }

		[[nodiscard]] std::vector<HeadingEntry> outline() const;
		[[nodiscard]] std::optional<HeadingEntry> headingForAnchor(const std::string& anchor) const;
		[[nodiscard]] std::optional<std::string> linkAt(size_t sourceOffset) const;
		[[nodiscard]] std::string withTaskToggled(size_t taskMarkOffset) const;

	private:
		void parse();

		std::string mSource;
		std::vector<Block> mBlocks;
	};

	[[nodiscard]] std::string headingAnchor(const std::string& title);
	[[nodiscard]] size_t lineStartOffset(const std::string& text, size_t offset);
	[[nodiscard]] size_t lineNumberAt(const std::string& text, size_t offset);
}
