#include "MarkdownDocument.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp::ui::markdown;

namespace {
	std::string sourceText(const MarkdownDocument& document, const SourceRange& range) {
		return document.source().substr(range.begin, range.end - range.begin);
	}
}

SUITE(MarkdownDocument) {
	TEST(HeadingsKeepLevelTextAndSourceRange) {
		MarkdownDocument document("# Title\n\n### Third level\n");

		CHECK_EQUAL(2u, document.blocks().size());
		const Block& title = document.blocks()[0];
		CHECK(title.kind == BlockKind::Heading);
		CHECK_EQUAL(1, title.headingLevel);
		CHECK_EQUAL("Title", title.plainText());
		CHECK_EQUAL("Title", sourceText(document, title.source));
		CHECK_EQUAL(3, document.blocks()[1].headingLevel);
	}

	TEST(InlineStylesSplitTheParagraphIntoRuns) {
		MarkdownDocument document("plain **bold** *em* `code` ~~gone~~ [link](http://a.b)");

		const std::vector<TextRun>& runs = document.blocks()[0].runs;
		CHECK_EQUAL("plain ", runs[0].text);
		CHECK_EQUAL("bold", runs[1].text);
		CHECK(runs[1].style.strong);
		CHECK(runs[3].style.emphasis);
		CHECK(runs[5].style.code);
		CHECK(runs[7].style.strikethrough);
		CHECK_EQUAL("link", runs[9].text);
		CHECK_EQUAL("http://a.b", runs[9].link);
		CHECK_EQUAL("bold", sourceText(document, runs[1].source));
	}

	TEST(SoftBreaksBecomeSpacesAndHardBreaksStaySeparate) {
		MarkdownDocument document("one\ntwo  \nthree");

		const Block& paragraph = document.blocks()[0];
		CHECK_EQUAL("one two three", paragraph.plainText());
		bool hasHardBreak = false;
		for (const TextRun& run : paragraph.runs)
			hasHardBreak |= run.hardBreak;
		CHECK(hasHardBreak);
	}

	TEST(EntitiesAreDecoded) {
		MarkdownDocument document("a &amp; b &copy;");
		CHECK_EQUAL("a & b \u00a9", document.blocks()[0].plainText());
	}

	TEST(ListsKeepNestingStartAndTasks) {
		MarkdownDocument document("3. first\n4. second\n\n- [x] done\n- [ ] todo\n  - nested\n");

		const Block& ordered = document.blocks()[0];
		CHECK(ordered.kind == BlockKind::OrderedList);
		CHECK_EQUAL(3u, ordered.listStart);
		CHECK_EQUAL(2u, ordered.children.size());

		const Block& tasks = document.blocks()[1];
		CHECK(tasks.kind == BlockKind::BulletList);
		CHECK(tasks.children[0].taskChecked == true);
		CHECK(tasks.children[1].taskChecked == false);
		CHECK_EQUAL('x', document.source()[tasks.children[0].taskMarkOffset]);
		CHECK(tasks.children[1].children.back().kind == BlockKind::BulletList);
	}

	TEST(TogglingATaskRewritesOnlyItsMark) {
		MarkdownDocument document("- [ ] write\n- [x] read\n");
		const std::vector<Block>& items = document.blocks()[0].children;

		const std::string checked = document.withTaskToggled(items[0].taskMarkOffset);
		CHECK_EQUAL("- [x] write\n- [x] read\n", checked);
		CHECK_EQUAL("- [ ] write\n- [ ] read\n", document.withTaskToggled(items[1].taskMarkOffset));
	}

	TEST(CodeBlocksKeepLanguageAndVerbatimText) {
		MarkdownDocument document("```cpp\nint x = 1;\n  return x;\n```\n");

		const Block& code = document.blocks()[0];
		CHECK(code.kind == BlockKind::Code);
		CHECK_EQUAL("cpp", code.codeLanguage);
		CHECK_EQUAL("int x = 1;\n  return x;", code.code);
	}

	TEST(TablesHaveHeaderRowsCellsAndAlignment) {
		MarkdownDocument document("| Name | Size |\n|:-----|-----:|\n| a | 1 |\n| b | 22 |\n");

		const Block& table = document.blocks()[0];
		CHECK(table.kind == BlockKind::Table);
		CHECK_EQUAL(3u, table.children.size());
		CHECK(table.children[0].headerRow);
		CHECK(!table.children[1].headerRow);
		CHECK_EQUAL("Size", table.children[0].children[1].plainText());
		CHECK(table.children[0].children[0].align == CellAlign::Left);
		CHECK(table.children[2].children[1].align == CellAlign::Right);
	}

	TEST(QuotesRulesAndAdmonitionsAreBlocks) {
		MarkdownDocument document("> quoted\n\n---\n\n> [!NOTE]\n> careful\n");

		CHECK(document.blocks()[0].kind == BlockKind::Quote);
		CHECK_EQUAL("quoted", document.blocks()[0].children[0].plainText());
		CHECK(document.blocks()[1].kind == BlockKind::Rule);
		CHECK_EQUAL("---", sourceText(document, document.blocks()[1].source));
		CHECK(document.blocks()[2].kind == BlockKind::Admonition);
		CHECK_EQUAL("note", document.blocks()[2].admonitionType);
	}

	TEST(ImagesBecomeLinkedRunsAndImageChildren) {
		MarkdownDocument document("see ![logo](img/logo.png)");

		const Block& paragraph = document.blocks()[0];
		CHECK_EQUAL("see logo", paragraph.plainText());
		CHECK_EQUAL("img/logo.png", paragraph.runs.back().link);
		CHECK(paragraph.children.back().kind == BlockKind::Image);
	}

	TEST(OutlineListsHeadingsInSourceOrder) {
		MarkdownDocument document("# A\ntext\n## B\n> # not top\n");

		const std::vector<HeadingEntry> outline = document.outline();
		CHECK_EQUAL(3u, outline.size());
		CHECK_EQUAL("B", outline[1].title);
		CHECK_EQUAL(2, outline[1].level);
		CHECK_EQUAL(document.source().find("B"), outline[1].sourceOffset);
	}

	TEST(BlockSourceRangesCoverTheirText) {
		MarkdownDocument document("para one\n\n- item **x**\n\n```\ncode\n```\n");

		for (const Block& block : document.blocks()) {
			CHECK(!block.source.isEmpty());
			CHECK(block.source.end <= document.source().size());
		}
		CHECK_EQUAL(0u, lineNumberAt(document.source(), document.blocks()[0].source.begin));
		CHECK_EQUAL(2u, lineNumberAt(document.source(), document.blocks()[1].source.begin));
	}

	TEST(EmptyAndMalformedInputDoNotCrash) {
		MarkdownDocument empty("");
		CHECK(empty.blocks().empty());

		MarkdownDocument broken("**unclosed *mixed `code\n| a |\n|-\n> ");
		CHECK(!broken.blocks().empty());
	}

	TEST(HeadingAnchorsFollowGithubRules) {
		CHECK_EQUAL("quotes-and-callouts", headingAnchor("Quotes and callouts"));
		CHECK_EQUAL("whats-new-in-v2", headingAnchor("What's new in v2?"));
		CHECK_EQUAL("snake_case---dash", headingAnchor("snake_case - dash"));

		MarkdownDocument document("# Intro\n\n## Usage\n\n## Usage\n");
		CHECK_EQUAL(document.source().find("Usage"), document.headingForAnchor("#usage")->sourceOffset);
		CHECK_EQUAL(document.source().rfind("Usage"), document.headingForAnchor("#usage-1")->sourceOffset);
		CHECK(!document.headingForAnchor("#missing"));
	}

	TEST(LinkAtFindsTheLinkUnderASourceOffset) {
		MarkdownDocument document("see [the site](https://a.b) and https://c.d now");
		const std::string& source = document.source();

		CHECK_EQUAL("https://a.b", *document.linkAt(source.find("[the")));
		CHECK_EQUAL("https://a.b", *document.linkAt(source.find("site")));
		CHECK_EQUAL("https://a.b", *document.linkAt(source.find("a.b)")));
		CHECK_EQUAL("https://c.d", *document.linkAt(source.find("c.d")));
		CHECK(!document.linkAt(source.find("see")));
		CHECK(!document.linkAt(source.find("now")));
	}
}
