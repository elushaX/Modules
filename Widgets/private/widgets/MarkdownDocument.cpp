#include "MarkdownDocument.hpp"

#include <md4c.h>

#include <algorithm>
#include <cctype>
#include <memory>

using namespace tp;
using namespace tp::ui::markdown;

namespace {
	constexpr unsigned kParserFlags = MD_DIALECT_GITHUB | MD_FLAG_UNDERLINE | MD_FLAG_HIGHLIGHT | MD_FLAG_NOHTMLSPANS;

	std::string attributeText(const MD_ATTRIBUTE& attribute) {
		if (!attribute.text) return {};
		return std::string(attribute.text, attribute.size);
	}

	CellAlign cellAlign(MD_ALIGN align) {
		switch (align) {
			case MD_ALIGN_LEFT: return CellAlign::Left;
			case MD_ALIGN_CENTER: return CellAlign::Center;
			case MD_ALIGN_RIGHT: return CellAlign::Right;
			default: return CellAlign::Default;
		}
	}

	std::string decodeEntity(const std::string& entity) {
		static const std::pair<const char*, const char*> kNamed[] = {
			{ "&amp;", "&" },        { "&lt;", "<" },         { "&gt;", ">" },          { "&quot;", "\"" },
			{ "&apos;", "'" },       { "&nbsp;", " " },       { "&copy;", "\u00a9" },   { "&reg;", "\u00ae" },
			{ "&mdash;", "\u2014" }, { "&ndash;", "\u2013" }, { "&hellip;", "\u2026" }, { "&rarr;", "\u2192" },
			{ "&larr;", "\u2190" },  { "&times;", "\u00d7" },
		};
		for (const auto& [name, value] : kNamed) {
			if (entity == name) return value;
		}
		return entity;
	}

	class DocumentBuilder {
	public:
		explicit DocumentBuilder(const std::string& source) :
			mSource(source) {
			mStack.push_back({});
		}

		std::vector<Block> finish() { return std::move(mStack.front().children); }

		void enterBlock(MD_BLOCKTYPE type, void* detail) {
			if (type == MD_BLOCK_DOC) return;
			if (type == MD_BLOCK_THEAD || type == MD_BLOCK_TBODY) {
				mInHeader = type == MD_BLOCK_THEAD;
				return;
			}

			Block block;
			describe(block, type, detail);
			mStack.push_back(std::move(block));
		}

		void leaveBlock(MD_BLOCKTYPE type) {
			if (type == MD_BLOCK_DOC || type == MD_BLOCK_THEAD || type == MD_BLOCK_TBODY) return;

			Block block = std::move(mStack.back());
			mStack.pop_back();
			finishBlock(block);
			Block& parent = mStack.back();
			if (!block.source.isEmpty()) parent.source.include(block.source.begin, block.source.end);
			parent.children.push_back(std::move(block));
		}

		void enterSpan(MD_SPANTYPE type, void* detail) {
			switch (type) {
				case MD_SPAN_STRONG: mStyle.strong = true; break;
				case MD_SPAN_EM: mStyle.emphasis = true; break;
				case MD_SPAN_CODE: mStyle.code = true; break;
				case MD_SPAN_DEL: mStyle.strikethrough = true; break;
				case MD_SPAN_U: mStyle.underline = true; break;
				case MD_SPAN_MARK: mStyle.highlight = true; break;
				case MD_SPAN_A: mLink = attributeText(static_cast<MD_SPAN_A_DETAIL*>(detail)->href); break;
				case MD_SPAN_WIKILINK: mLink = attributeText(static_cast<MD_SPAN_WIKILINK_DETAIL*>(detail)->target); break;
				case MD_SPAN_IMG: enterImage(*static_cast<MD_SPAN_IMG_DETAIL*>(detail)); break;
				default: break;
			}
		}

		void leaveSpan(MD_SPANTYPE type) {
			switch (type) {
				case MD_SPAN_STRONG: mStyle.strong = false; break;
				case MD_SPAN_EM: mStyle.emphasis = false; break;
				case MD_SPAN_CODE: mStyle.code = false; break;
				case MD_SPAN_DEL: mStyle.strikethrough = false; break;
				case MD_SPAN_U: mStyle.underline = false; break;
				case MD_SPAN_MARK: mStyle.highlight = false; break;
				case MD_SPAN_A:
				case MD_SPAN_WIKILINK: mLink.clear(); break;
				case MD_SPAN_IMG: leaveImage(); break;
				default: break;
			}
		}

		void text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size) {
			const SourceRange source = sourceOf(text, size);
			Block& block = mStack.back();
			if (!source.isEmpty()) block.source.include(source.begin, source.end);

			if (mImage) {
				mImage->imageAlt.append(text, size);
				if (!source.isEmpty()) mImage->source.include(source.begin, source.end);
				return;
			}
			if (block.kind == BlockKind::Code || block.kind == BlockKind::Html) {
				block.code.append(text, size);
				return;
			}

			switch (type) {
				case MD_TEXT_BR: appendBreak(block, true); return;
				case MD_TEXT_SOFTBR: appendText(block, " ", source); return;
				case MD_TEXT_NULLCHAR: appendText(block, "\ufffd", source); return;
				case MD_TEXT_ENTITY: appendText(block, decodeEntity(std::string(text, size)), source); return;
				default: appendText(block, std::string(text, size), source); return;
			}
		}

	private:
		void describe(Block& block, MD_BLOCKTYPE type, void* detail) {
			switch (type) {
				case MD_BLOCK_QUOTE: block.kind = BlockKind::Quote; break;
				case MD_BLOCK_UL:
					block.kind = BlockKind::BulletList;
					block.tightList = static_cast<MD_BLOCK_UL_DETAIL*>(detail)->is_tight != 0;
					break;
				case MD_BLOCK_OL:
					{
						const auto* ordered = static_cast<MD_BLOCK_OL_DETAIL*>(detail);
						block.kind = BlockKind::OrderedList;
						block.tightList = ordered->is_tight != 0;
						block.listStart = ordered->start;
						break;
					}
				case MD_BLOCK_LI: describeListItem(block, *static_cast<MD_BLOCK_LI_DETAIL*>(detail)); break;
				case MD_BLOCK_HR: block.kind = BlockKind::Rule; break;
				case MD_BLOCK_H:
					block.kind = BlockKind::Heading;
					block.headingLevel = (int) static_cast<MD_BLOCK_H_DETAIL*>(detail)->level;
					break;
				case MD_BLOCK_CODE:
					block.kind = BlockKind::Code;
					block.codeLanguage = attributeText(static_cast<MD_BLOCK_CODE_DETAIL*>(detail)->lang);
					break;
				case MD_BLOCK_HTML: block.kind = BlockKind::Html; break;
				case MD_BLOCK_TABLE: block.kind = BlockKind::Table; break;
				case MD_BLOCK_TR:
					block.kind = BlockKind::TableRow;
					block.headerRow = mInHeader;
					break;
				case MD_BLOCK_TH:
				case MD_BLOCK_TD:
					block.kind = BlockKind::TableCell;
					block.headerRow = type == MD_BLOCK_TH;
					block.align = cellAlign(static_cast<MD_BLOCK_TD_DETAIL*>(detail)->align);
					break;
				case MD_BLOCK_ADMONITION:
					block.kind = BlockKind::Admonition;
					block.admonitionType = attributeText(static_cast<MD_BLOCK_ADMONITION_DETAIL*>(detail)->type);
					break;
				default: block.kind = BlockKind::Paragraph; break;
			}
		}

		void describeListItem(Block& block, const MD_BLOCK_LI_DETAIL& item) {
			block.kind = BlockKind::ListItem;
			if (!item.is_task) return;

			block.taskChecked = item.task_mark == 'x' || item.task_mark == 'X';
			block.taskMarkOffset = item.task_mark_offset;
			block.source.include(item.task_mark_offset, item.task_mark_offset + 1);
		}

		void finishBlock(Block& block) const {
			if (block.kind == BlockKind::Code && !block.code.empty() && block.code.back() == '\n') block.code.pop_back();
			if (block.kind == BlockKind::Rule || block.source.isEmpty()) block.source = lineRangeNear(block);
		}

		[[nodiscard]] SourceRange lineRangeNear(const Block& block) const {
			const Block& parent = mStack.back();
			const size_t anchor = !parent.children.empty() ? parent.children.back().source.end : parent.source.begin;
			if (block.kind != BlockKind::Rule) return { anchor, anchor };

			size_t start = anchor;
			while (start < mSource.size() && (mSource[start] == '\n' || mSource[start] == ' '))
				start++;
			const size_t end = std::min(mSource.find('\n', start), mSource.size());
			return { start, end };
		}

		void enterImage(const MD_SPAN_IMG_DETAIL& image) {
			Block block;
			block.kind = BlockKind::Image;
			block.imageSource = attributeText(image.src);
			mImageOwner = mStack.size() - 1;
			mImage = std::make_unique<Block>(std::move(block));
		}

		void leaveImage() {
			if (!mImage) return;

			Block& owner = mStack[mImageOwner];
			TextRun run;
			run.text = mImage->imageAlt.empty() ? "image" : mImage->imageAlt;
			run.style = mStyle;
			run.style.emphasis = true;
			run.link = mImage->imageSource;
			run.source = mImage->source;
			owner.runs.push_back(run);
			owner.children.push_back(std::move(*mImage));
			mImage.reset();
		}

		void appendText(Block& block, const std::string& text, const SourceRange& source) {
			if (!block.runs.empty()) {
				TextRun& last = block.runs.back();
				if (!last.hardBreak && last.style == mStyle && last.link == mLink) {
					last.text += text;
					if (!source.isEmpty()) last.source.include(source.begin, source.end);
					return;
				}
			}
			block.runs.push_back({ text, mStyle, mLink, source, false });
		}

		void appendBreak(Block& block, bool hard) {
			TextRun run;
			run.style = mStyle;
			run.hardBreak = hard;
			block.runs.push_back(run);
		}

		[[nodiscard]] SourceRange sourceOf(const MD_CHAR* text, MD_SIZE size) const {
			const char* begin = mSource.data();
			if (text < begin || text + size > begin + mSource.size()) return {};
			const size_t offset = (size_t) (text - begin);
			return { offset, offset + size };
		}

		const std::string& mSource;
		std::vector<Block> mStack;
		InlineStyle mStyle;
		std::string mLink;
		bool mInHeader = false;
		std::unique_ptr<Block> mImage;
		size_t mImageOwner = 0;
	};

	int onEnterBlock(MD_BLOCKTYPE type, void* detail, void* user) {
		static_cast<DocumentBuilder*>(user)->enterBlock(type, detail);
		return 0;
	}

	int onLeaveBlock(MD_BLOCKTYPE type, void*, void* user) {
		static_cast<DocumentBuilder*>(user)->leaveBlock(type);
		return 0;
	}

	int onEnterSpan(MD_SPANTYPE type, void* detail, void* user) {
		static_cast<DocumentBuilder*>(user)->enterSpan(type, detail);
		return 0;
	}

	int onLeaveSpan(MD_SPANTYPE type, void*, void* user) {
		static_cast<DocumentBuilder*>(user)->leaveSpan(type);
		return 0;
	}

	int onText(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* user) {
		static_cast<DocumentBuilder*>(user)->text(type, text, size);
		return 0;
	}

	void collectHeadings(const std::vector<Block>& blocks, std::vector<HeadingEntry>& outline) {
		for (const Block& block : blocks) {
			if (block.kind == BlockKind::Heading) {
				const std::string title = block.plainText();
				outline.push_back({ block.headingLevel, title, headingAnchor(title), block.source.begin });
			}
			collectHeadings(block.children, outline);
		}
	}

	size_t linkSourceEnd(const std::string& source, size_t textEnd) {
		if (textEnd + 1 >= source.size() || source[textEnd] != ']' || source[textEnd + 1] != '(') return textEnd;
		const size_t close = source.find(')', textEnd);
		const size_t lineEnd = source.find('\n', textEnd);
		if (close == std::string::npos || (lineEnd != std::string::npos && close > lineEnd)) return textEnd;
		return close + 1;
	}

	std::optional<std::string> findLink(const std::vector<Block>& blocks, const std::string& source, size_t offset) {
		for (const Block& block : blocks) {
			if (!block.source.isEmpty() && (offset < block.source.begin || offset > block.source.end)) continue;
			for (const TextRun& run : block.runs) {
				if (run.link.empty() || run.source.isEmpty()) continue;
				const size_t begin =
					run.source.begin > 0 && source[run.source.begin - 1] == '[' ? run.source.begin - 1 : run.source.begin;
				if (offset >= begin && offset < linkSourceEnd(source, run.source.end)) return run.link;
			}
			if (std::optional<std::string> nested = findLink(block.children, source, offset)) return nested;
		}
		return std::nullopt;
	}
}

void SourceRange::include(size_t from, size_t to) {
	if (isEmpty()) {
		begin = from;
		end = to;
		return;
	}
	begin = std::min(begin, from);
	end = std::max(end, to);
}

std::string Block::plainText() const {
	std::string text;
	for (const TextRun& run : runs)
		text += run.hardBreak ? std::string(" ") : run.text;
	return text;
}

MarkdownDocument::MarkdownDocument(std::string source) { setSource(std::move(source)); }

void MarkdownDocument::setSource(std::string source) {
	mSource = std::move(source);
	parse();
}

void MarkdownDocument::parse() {
	DocumentBuilder builder(mSource);
	MD_PARSER parser = {};
	parser.flags = kParserFlags;
	parser.enter_block = onEnterBlock;
	parser.leave_block = onLeaveBlock;
	parser.enter_span = onEnterSpan;
	parser.leave_span = onLeaveSpan;
	parser.text = onText;
	md_parse(mSource.data(), (MD_SIZE) mSource.size(), &parser, &builder);
	mBlocks = builder.finish();
}

std::vector<HeadingEntry> MarkdownDocument::outline() const {
	std::vector<HeadingEntry> outline;
	collectHeadings(mBlocks, outline);
	return outline;
}

std::optional<HeadingEntry> MarkdownDocument::headingForAnchor(const std::string& anchor) const {
	const std::string wanted = anchor.starts_with('#') ? anchor.substr(1) : anchor;
	std::vector<std::string> seen;
	for (const HeadingEntry& heading : outline()) {
		const size_t repeats = (size_t) std::count(seen.begin(), seen.end(), heading.anchor);
		seen.push_back(heading.anchor);
		const std::string unique = repeats == 0 ? heading.anchor : heading.anchor + "-" + std::to_string(repeats);
		if (unique == wanted || unique == headingAnchor(wanted)) return heading;
	}
	return std::nullopt;
}

std::optional<std::string> MarkdownDocument::linkAt(size_t sourceOffset) const {
	return findLink(mBlocks, mSource, sourceOffset);
}

std::string tp::ui::markdown::headingAnchor(const std::string& title) {
	std::string anchor;
	for (const char character : title) {
		const auto byte = (unsigned char) character;
		if (byte >= 0x80) {
			anchor += character;
		} else if (std::isalnum(byte) || character == '-' || character == '_') {
			anchor += (char) std::tolower(byte);
		} else if (character == ' ') {
			anchor += '-';
		}
	}
	return anchor;
}

std::string MarkdownDocument::withTaskToggled(size_t taskMarkOffset) const {
	std::string toggled = mSource;
	if (taskMarkOffset >= toggled.size()) return toggled;

	char& mark = toggled[taskMarkOffset];
	if (mark == ' ') {
		mark = 'x';
	} else if (mark == 'x' || mark == 'X') {
		mark = ' ';
	}
	return toggled;
}

size_t tp::ui::markdown::lineStartOffset(const std::string& text, size_t offset) {
	offset = std::min(offset, text.size());
	while (offset > 0 && text[offset - 1] != '\n')
		offset--;
	return offset;
}

size_t tp::ui::markdown::lineNumberAt(const std::string& text, size_t offset) {
	offset = std::min(offset, text.size());
	return (size_t) std::count(text.begin(), text.begin() + (std::ptrdiff_t) offset, '\n');
}
