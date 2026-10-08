#include "BoxLayout.hpp"
#include "RichText.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	TextSpan span(std::string text, FontFace face = FontFace::Regular) {
		TextSpan result;
		result.text = std::move(text);
		result.face = face;
		return result;
	}

	TextSpan lineBreak() {
		TextSpan result;
		result.lineBreak = true;
		return result;
	}

	TextSpan linkSpan(std::string text, std::string target) {
		TextSpan result = span(std::move(text));
		result.link = std::move(target);
		return result;
	}

	struct RichScene {
		Widget root;
		Widget column;
		RichText text;

		explicit RichScene(std::vector<TextSpan> spans, halnf width = 400) {
			root.emplaceLayout<RowLayout>().setCrossAlign(CrossAlign::Start);
			column.emplaceLayout<ColumnLayout>();
			column.setSizing(AxisSizing::fixed(width), AxisSizing::fit());
			text.setSpans(std::move(spans));
			column.addChild(text);
			root.addChild(column);
		}
	};

	std::vector<halnf> lineTops(const std::vector<PlacedSpanPiece>& pieces) {
		std::vector<halnf> tops;
		for (const PlacedSpanPiece& piece : pieces) {
			if (tops.empty() || tops.back() != piece.box.pos.y) tops.push_back(piece.box.pos.y);
		}
		return tops;
	}
}

SUITE(RichTextWidget) {
	TEST(SpansOnOneLineKeepTheirOrderAndFaces) {
		RichScene scene({ span("plain "), span("bold", FontFace::Bold), span(" mono", FontFace::Monospace) });
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		CHECK(painter.hasText("plain "));
		CHECK(painter.findText("bold")->face == FontFace::Bold);
		CHECK(painter.findText(" mono")->face == FontFace::Monospace);
		CHECK(painter.findText("plain ")->rect.pos.x < painter.findText("bold")->rect.pos.x);
		CHECK_EQUAL(1u, lineTops(scene.text.piecesFor(400)).size());
	}

	TEST(LongTextWrapsAtWordsWithinTheWidth) {
		RichScene scene({ span("alpha beta gamma delta epsilon zeta eta theta iota kappa") }, 120);
		UiDriver driver(scene.root);

		const std::vector<PlacedSpanPiece>& pieces = scene.text.piecesFor(120);
		CHECK(lineTops(pieces).size() > 2);
		for (const PlacedSpanPiece& piece : pieces) {
			CHECK(piece.box.pos.x + MonospaceTextMetrics().textWidth(piece.text, 13) <= 120 + 7);
			CHECK(piece.text.front() != ' ');
		}
		CHECK_CLOSE((halnf) lineTops(pieces).size() * 18.0f, scene.text.size().y, 1.0f);
	}

	TEST(AWordWiderThanTheLineIsSplit) {
		RichScene scene({ span("supercalifragilisticexpialidocious") }, 60);
		UiDriver driver(scene.root);

		const std::vector<PlacedSpanPiece>& pieces = scene.text.piecesFor(60);
		CHECK(pieces.size() > 2);
		std::string joined;
		for (const PlacedSpanPiece& piece : pieces)
			joined += piece.text;
		CHECK_EQUAL("supercalifragilisticexpialidocious", joined);
	}

	TEST(LineBreakSpansStartANewLine) {
		RichScene scene({ span("first."), lineBreak(), span("Second line") });
		UiDriver driver(scene.root);

		const std::vector<halnf> tops = lineTops(scene.text.piecesFor(400));
		CHECK_EQUAL(2u, tops.size());
		CHECK_EQUAL("first.\nSecond line", scene.text.plainText());
	}

	TEST(ClickingALinkReportsItsTarget) {
		RichScene scene({ span("see "), linkSpan("docs", "https://example.com") });
		std::string activated;
		scene.text.setOnLinkActivated([&](const std::string& target) { activated = target; });
		UiDriver driver(scene.root);

		const PlacedSpanPiece& docs = scene.text.piecesFor(400).back();
		driver.click(scene.text.windowPosition() + docs.box.center());

		CHECK_EQUAL("https://example.com", activated);
	}

	TEST(HoveringALinkShowsAPointerAndUnderline) {
		RichScene scene({ linkSpan("docs", "https://example.com") });
		UiDriver driver(scene.root);

		const PlacedSpanPiece& docs = scene.text.piecesFor(400).front();
		driver.moveTo(scene.text.windowPosition() + docs.box.center());

		CHECK(scene.text.cursor() == CursorShape::Pointer);
		bool underlined = false;
		for (const PaintCommand& command : driver.paint().commands())
			underlined |= command.kind == PaintCommand::Kind::Line;
		CHECK(underlined);

		driver.moveTo(Vec2F(399.0f, 500.0f));
		CHECK(scene.text.cursor() == CursorShape::Default);
	}

	TEST(ClickingPlainTextReportsItsSourceOffset) {
		TextSpan plain = span("hello");
		plain.sourceOffset = 42;
		RichScene scene({ plain });
		size_t reported = 0;
		scene.text.setOnSourceActivated([&](size_t offset) { reported = offset; });
		UiDriver driver(scene.root);

		driver.click(scene.text.windowPosition() + scene.text.piecesFor(400).front().box.center());

		CHECK_EQUAL(42u, reported);
	}

	TEST(BackgroundSpansPaintARoundedChip) {
		TextSpan code = span("x()", FontFace::Monospace);
		code.background = RGBA(0.2f, 0.2f, 0.2f, 1);
		RichScene scene({ span("call "), code });
		UiDriver driver(scene.root);

		bool chip = false;
		for (const PaintCommand& command : driver.paint().commands())
			chip |= command.kind == PaintCommand::Kind::FillRect && command.cornerRadius > 0;
		CHECK(chip);
	}

	TEST(CenteredAlignmentSharesTheSlack) {
		RichScene scene({ span("ab") }, 200);
		scene.text.setAlignment(TextAlign::Center);
		UiDriver driver(scene.root);

		const PlacedSpanPiece& piece = scene.text.piecesFor(200).front();
		const halnf width = MonospaceTextMetrics().textWidth("ab", 13);
		CHECK_CLOSE((200 - width) / 2, piece.box.pos.x, 1.0f);
	}
}
