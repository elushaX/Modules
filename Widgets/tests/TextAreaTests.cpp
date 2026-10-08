#include "BoxLayout.hpp"
#include "ColorPicker.hpp"
#include "FakeWindowHost.hpp"
#include "TextArea.hpp"
#include "TextEditBuffer.hpp"
#include "TextField.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

#include <random>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	Modifiers control(bool shift = false) {
		Modifiers modifiers;
		modifiers.control = true;
		modifiers.shift = shift;
		return modifiers;
	}

	Modifiers shift() {
		Modifiers modifiers;
		modifiers.shift = true;
		return modifiers;
	}

	struct AreaScene {
		Widget root;
		TextArea area;
		TextField after;
		FakeWindowHost host;
		std::vector<std::string> edits;
		std::vector<std::string> commits;

		explicit AreaScene(std::string text = {}, halnf width = 200) {
			root.emplaceLayout<ColumnLayout>(10);
			area.setSizing(Axis::Horizontal, AxisSizing::fixed(width));
			area.setText(std::move(text));
			area.setOnEdited([this](const std::string& text) { edits.push_back(text); });
			area.setOnCommitted([this](const std::string& text) { commits.push_back(text); });
			root.addChild(area);
			root.addChild(after);
		}

		TextAreaEditor& editor() { return area.editor(); }

		Vec2F pointAt(size_t line, halnf x) {
			const halnf y = 4 + editor().lineHeight() * (halnf(line) + 0.5f);
			return editor().windowPosition() + Vec2F(x, y);
		}

		void focus(UiDriver& driver) {
			driver.ui().setWindowHost(&host);
			driver.click(pointAt(0, 4));
		}
	};
}

SUITE(TextEditBuffer) {
	TEST(SingleLineRejectsNewlinesAndControlCharacters) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Single);
		CHECK(buffer.insert(
			"a\nb\x01"
			"c"
		));
		CHECK_EQUAL("abc", buffer.text());
	}

	TEST(MultipleLinesKeepNewlinesAndDropCarriageReturns) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Multiple);
		buffer.insert("one\r\ntwo\tthree");
		CHECK_EQUAL("one\ntwo three", buffer.text());
	}

	TEST(InsertReplacesSelection) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Multiple, "hello world");
		buffer.select(0, 5);
		buffer.insert("bye");
		CHECK_EQUAL("bye world", buffer.text());
		CHECK_EQUAL(3u, buffer.caret());
		CHECK(!buffer.hasSelection());
	}

	TEST(MaximumLengthCountsReplacedSelection) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Single, "abcd");
		buffer.setMaximumLength(4);
		buffer.select(1, 3);
		CHECK(buffer.insert("XY"));
		CHECK_EQUAL("aXYd", buffer.text());
	}

	TEST(MaximumLengthNeverSplitsUtf8) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Single);
		buffer.setMaximumLength(3);
		buffer.insert("a\xD0\x96\xD0\x96");
		CHECK_EQUAL("a\xD0\x96", buffer.text());
	}

	TEST(CaretSnapsToCharacterBoundary) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Single, "\xD0\x96x");
		buffer.setCaret(1);
		CHECK_EQUAL(0u, buffer.caret());
		CHECK(buffer.eraseForward(false));
		CHECK_EQUAL("x", buffer.text());
	}

	TEST(WordAndLineNavigation) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Multiple, "first line\nsecond  word");
		CHECK_EQUAL(11u, buffer.lineStart(15));
		CHECK_EQUAL(10u, buffer.lineEnd(3));
		CHECK_EQUAL(buffer.text().size(), buffer.lineEnd(12));
		CHECK_EQUAL(19u, buffer.previousWord(23));
		CHECK_EQUAL(17u, buffer.nextWord(11));
	}

	TEST(EraseBackwardWholeWord) {
		TextEditBuffer buffer(TextEditBuffer::Lines::Single, "alpha beta");
		CHECK(buffer.eraseBackward(true));
		CHECK_EQUAL("alpha ", buffer.text());
		CHECK(!TextEditBuffer().eraseBackward(false));
	}
}

SUITE(TextAreaLayout) {
	TEST(HardBreaksMakeLines) {
		AreaScene scene("a\nbb\n\nccc");
		UiDriver driver(scene.root);

		REQUIRE CHECK_EQUAL(4u, scene.editor().lines().size());
		CHECK_EQUAL(0u, scene.editor().lines()[2].end - scene.editor().lines()[2].start);
	}

	TEST(TrailingNewlineAddsEmptyLine) {
		AreaScene scene("abc\n");
		UiDriver driver(scene.root);
		CHECK_EQUAL(2u, scene.editor().lines().size());
	}

	TEST(LongParagraphWrapsAtSpaces) {
		AreaScene scene("aaaa bbbb cccc dddd eeee ffff gggg hhhh", 120);
		UiDriver driver(scene.root);

		const std::vector<TextLayoutLine>& lines = scene.editor().lines();
		REQUIRE CHECK(lines.size() > 1);
		for (const TextLayoutLine& line : lines) {
			const std::string text = scene.area.text().substr(line.start, line.end - line.start);
			CHECK(text.empty() || text.front() != ' ');
		}
		CHECK_EQUAL(scene.area.text().size(), lines.back().end);
	}

	TEST(UnbrokenWordWrapsByCharacter) {
		AreaScene scene(std::string(80, 'x'), 120);
		UiDriver driver(scene.root);

		const std::vector<TextLayoutLine>& lines = scene.editor().lines();
		CHECK(lines.size() > 2);
		for (size_t index = 1; index < lines.size(); index++)
			CHECK_EQUAL(lines[index - 1].end, lines[index].start);
	}

	TEST(EditorHeightFollowsLineCount) {
		AreaScene scene("1\n2\n3\n4\n5\n6\n7\n8\n9\n10");
		UiDriver driver(scene.root);

		CHECK_CLOSE(10 * scene.editor().lineHeight(), scene.editor().size().y, scene.editor().lineHeight());
		CHECK(scene.area.scroll().isScrollable());
	}
}

SUITE(TextAreaEditing) {
	TEST(TypingAndEnterInsertNewline) {
		AreaScene scene;
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.type("one");
		driver.key(InputID::ENTER);
		driver.type("two");

		CHECK_EQUAL("one\ntwo", scene.area.text());
		CHECK_EQUAL(2u, scene.editor().lines().size());
		CHECK_EQUAL("one\ntwo", scene.edits.back());
	}

	TEST(CtrlEnterCommitsWithoutNewline) {
		AreaScene scene("text");
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.key(InputID::ENTER, control());
		CHECK_EQUAL("text", scene.area.text());
		REQUIRE CHECK_EQUAL(1u, scene.commits.size());
	}

	TEST(LosingFocusCommits) {
		AreaScene scene("text");
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.click(scene.after);
		CHECK(!scene.editor().isEditing());
		CHECK_EQUAL(1u, scene.commits.size());
	}

	TEST(ClickPlacesCaretOnClickedLine) {
		AreaScene scene("first\nsecond\nthird");
		UiDriver driver(scene.root);

		driver.click(scene.pointAt(1, 190));
		CHECK_EQUAL(12u, scene.editor().buffer().caret());
		driver.click(scene.pointAt(2, 0));
		CHECK_EQUAL(13u, scene.editor().buffer().caret());
	}

	TEST(UpAndDownKeepColumn) {
		AreaScene scene("abcdef\nab\nabcdef");
		UiDriver driver(scene.root);
		scene.focus(driver);

		scene.editor().setCaret(5);
		driver.key(InputID::DOWN);
		CHECK_EQUAL(9u, scene.editor().buffer().caret());
		driver.key(InputID::DOWN);
		CHECK_EQUAL(15u, scene.editor().buffer().caret());
		driver.key(InputID::UP);
		driver.key(InputID::UP);
		CHECK_EQUAL(5u, scene.editor().buffer().caret());
	}

	TEST(UpOnFirstLineGoesToStartAndDownOnLastToEnd) {
		AreaScene scene("abc\ndef");
		UiDriver driver(scene.root);
		scene.focus(driver);

		scene.editor().setCaret(2);
		driver.key(InputID::UP);
		CHECK_EQUAL(0u, scene.editor().buffer().caret());
		driver.key(InputID::DOWN);
		driver.key(InputID::DOWN);
		CHECK_EQUAL(7u, scene.editor().buffer().caret());
	}

	TEST(ShiftDownSelectsAcrossLines) {
		AreaScene scene("abc\ndef");
		UiDriver driver(scene.root);
		scene.focus(driver);

		scene.editor().setCaret(1);
		driver.key(InputID::DOWN, shift());
		CHECK_EQUAL("bc\nd", scene.editor().buffer().selectedText());
	}

	TEST(HomeEndUseVisualLineAndCtrlUsesDocument) {
		AreaScene scene("abc\ndef");
		UiDriver driver(scene.root);
		scene.focus(driver);

		scene.editor().setCaret(5);
		driver.key(InputID::HOME);
		CHECK_EQUAL(4u, scene.editor().buffer().caret());
		driver.key(InputID::END);
		CHECK_EQUAL(7u, scene.editor().buffer().caret());
		driver.key(InputID::HOME, control());
		CHECK_EQUAL(0u, scene.editor().buffer().caret());
		driver.key(InputID::END, control());
		CHECK_EQUAL(7u, scene.editor().buffer().caret());
	}

	TEST(BackspaceJoinsLines) {
		AreaScene scene("abc\ndef");
		UiDriver driver(scene.root);
		scene.focus(driver);

		scene.editor().setCaret(4);
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("abcdef", scene.area.text());
		CHECK_EQUAL(1u, scene.editor().lines().size());
	}

	TEST(CopyCutPasteUseClipboard) {
		AreaScene scene("one\ntwo");
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.key(InputID::A, control());
		driver.key(InputID::C, control());
		CHECK_EQUAL("one\ntwo", scene.host.clipboard);

		driver.key(InputID::X, control());
		CHECK_EQUAL("", scene.area.text());
		driver.key(InputID::V, control());
		driver.key(InputID::V, control());
		CHECK_EQUAL("one\ntwoone\ntwo", scene.area.text());
	}

	TEST(DragSelectsAcrossLines) {
		AreaScene scene("first\nsecond");
		UiDriver driver(scene.root);

		driver.drag(scene.pointAt(0, 8 + 6.5f * 1.5f), scene.pointAt(1, 190));
		CHECK_EQUAL("irst\nsecond", scene.editor().buffer().selectedText());
	}

	TEST(DoubleClickSelectsWordTripleSelectsLine) {
		AreaScene scene("alpha beta\ngamma");
		UiDriver driver(scene.root);
		const Vec2F point = scene.pointAt(0, 8 + 6.5f * 7.5f);

		driver.click(point);
		driver.click(point);
		CHECK_EQUAL("beta", scene.editor().buffer().selectedText());

		driver.click(point);
		CHECK_EQUAL("alpha beta", scene.editor().buffer().selectedText());
	}

	TEST(EscapeLeavesEditing) {
		AreaScene scene("text");
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.key(InputID::ESCAPE);
		CHECK(!scene.editor().isEditing());
	}

	TEST(DownThroughSoftWrappedLinesVisitsEachVisualLine) {
		AreaScene scene("aaaa bbbb cccc dddd eeee ffff", 100);
		UiDriver driver(scene.root);
		scene.focus(driver);

		const size_t lineCount = scene.editor().lines().size();
		REQUIRE CHECK(lineCount > 2);
		scene.editor().setCaret(0);
		for (size_t line = 1; line < lineCount; line++) {
			driver.key(InputID::DOWN);
			CHECK_EQUAL(line, scene.editor().lineOf(scene.editor().buffer().caret()));
		}
	}

	TEST(ClickAtEndOfSoftWrappedLineStaysOnThatLine) {
		AreaScene scene("aaaa bbbb cccc dddd eeee ffff", 100);
		UiDriver driver(scene.root);

		driver.click(scene.pointAt(0, 95));
		CHECK_EQUAL(0u, scene.editor().lineOf(scene.editor().buffer().caret()));
	}

	TEST(Utf8CharactersMoveAndEraseAsWholes) {
		AreaScene scene("\xD0\x96\xD0\x96\n\xD0\x96");
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.key(InputID::END, control());
		driver.key(InputID::LEFT);
		CHECK_EQUAL(5u, scene.editor().buffer().caret());
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("\xD0\x96\xD0\x96\xD0\x96", scene.area.text());
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("\xD0\x96\xD0\x96", scene.area.text());
	}

	TEST(EmptyAreaHasOneLineAndShowsPlaceholder) {
		AreaScene scene;
		scene.area.setPlaceholder("Describe it");
		UiDriver driver(scene.root);

		CHECK_EQUAL(1u, scene.editor().lines().size());
		CHECK(driver.paint().findText("Describe it") != nullptr);
		scene.focus(driver);
		driver.key(InputID::UP);
		driver.key(InputID::DOWN);
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("", scene.area.text());
	}

	TEST(SetTextDoesNotReportEdit) {
		AreaScene scene;
		UiDriver driver(scene.root);
		scene.area.setText("programmatic");
		CHECK_EQUAL(0u, scene.edits.size());
	}
}

SUITE(TextAreaScrollingAndResize) {
	TEST(TypingPastBottomScrollsToCaret) {
		AreaScene scene;
		UiDriver driver(scene.root);
		scene.focus(driver);

		for (int line = 0; line < 20; line++) {
			driver.type("line");
			driver.key(InputID::ENTER);
		}
		driver.settle();

		const RectF caret = scene.editor().caretRect();
		const halnf offset = scene.area.scroll().offset();
		CHECK(offset > 0);
		CHECK(caret.pos.y + caret.size.y <= offset + scene.area.scroll().size().y + 0.5f);
	}

	TEST(CtrlHomeScrollsBackToTop) {
		AreaScene scene("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");
		UiDriver driver(scene.root);
		scene.focus(driver);

		driver.key(InputID::END, control());
		driver.settle();
		CHECK(scene.area.scroll().offset() > 0);

		driver.key(InputID::HOME, control());
		driver.settle();
		CHECK_CLOSE(0, scene.area.scroll().offset(), 0.5f);
	}

	TEST(WheelScrollsLongText) {
		AreaScene scene("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");
		UiDriver driver(scene.root);

		driver.moveTo(scene.pointAt(1, 20));
		driver.wheel(-1);
		driver.settle();
		CHECK(scene.area.scroll().offset() > 0);
	}

	TEST(DraggingHandleResizesHeightWithinRange) {
		AreaScene scene;
		scene.area.setHeightRange(60, 200);
		scene.area.setBoxHeight(100);
		UiDriver driver(scene.root);

		const Vec2F handle = scene.area.resizeHandle().windowRect().center();
		driver.drag(handle, handle + Vec2F(0.0f, 50.0f));
		CHECK_CLOSE(150, scene.area.boxHeight(), 0.5f);
		CHECK_CLOSE(150, scene.area.size().y, 0.5f);

		const Vec2F moved = scene.area.resizeHandle().windowRect().center();
		driver.drag(moved, moved + Vec2F(0.0f, 500.0f));
		CHECK_CLOSE(200, scene.area.boxHeight(), 0.5f);

		const Vec2F top = scene.area.resizeHandle().windowRect().center();
		driver.drag(top, top - Vec2F(0.0f, 500.0f));
		CHECK_CLOSE(60, scene.area.boxHeight(), 0.5f);
	}

	TEST(ShrinkingWhileScrolledKeepsOffsetInRange) {
		AreaScene scene("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12");
		scene.area.setBoxHeight(150);
		UiDriver driver(scene.root);
		scene.focus(driver);
		driver.key(InputID::END, control());
		driver.settle();

		scene.area.setBoxHeight(400);
		driver.settle();
		CHECK(scene.area.scroll().offset() <= scene.area.scroll().maximumOffset() + 0.5f);
		CHECK(!scene.area.scroll().isScrollable());
	}

	TEST(HandleStaysAtBottomEdge) {
		AreaScene scene;
		scene.area.setBoxHeight(120);
		UiDriver driver(scene.root);

		const RectF area = scene.area.windowRect();
		const RectF handle = scene.area.resizeHandle().windowRect();
		CHECK_CLOSE(area.pos.y + area.size.y, handle.pos.y + handle.size.y, 0.5f);
	}
}

SUITE(TextAreaStress) {
	TEST(RandomEditingMatchesReferenceModelAndKeepsLayoutConsistent) {
		AreaScene scene({}, 140);
		UiDriver driver(scene.root);
		scene.focus(driver);

		std::mt19937 random(1234);
		std::string reference;
		size_t caret = 0;
		const char* fragments[] = { "a", "word ", "\n", "xy z", "\xD0\x96" };

		for (int step = 0; step < 400; step++) {
			const int action = int(random() % 6);
			if (action <= 2) {
				const std::string fragment = fragments[random() % 5];
				driver.type(fragment);
				reference.insert(caret, fragment);
				caret += fragment.size();
			} else if (action == 3) {
				driver.key(InputID::BACKSPACE);
				if (caret > 0) {
					size_t from = caret - 1;
					while (from > 0 && (((unsigned char) reference[from]) & 0xC0) == 0x80)
						from--;
					reference.erase(from, caret - from);
					caret = from;
				}
			} else if (action == 4) {
				driver.key(InputID::HOME, control());
				caret = 0;
			} else {
				driver.key(InputID::END, control());
				caret = reference.size();
			}

			REQUIRE CHECK_EQUAL(reference, scene.area.text());
			REQUIRE CHECK_EQUAL(caret, scene.editor().buffer().caret());

			const std::vector<TextLayoutLine>& lines = scene.editor().lines();
			CHECK_EQUAL(0u, lines.front().start);
			CHECK_EQUAL(reference.size(), lines.back().end);
			for (size_t index = 1; index < lines.size(); index++)
				CHECK(lines[index - 1].end <= lines[index].start);
		}
	}

	TEST(ArrowWalkVisitsEveryBoundaryOnceEachWay) {
		AreaScene scene("ab\xD0\x96 cd\nef  gh\n\nlong line that wraps around", 100);
		UiDriver driver(scene.root);
		scene.focus(driver);
		driver.key(InputID::HOME, control());

		std::vector<size_t> forward = { 0 };
		for (int step = 0; step < 200 && forward.back() < scene.area.text().size(); step++) {
			driver.key(InputID::RIGHT);
			forward.push_back(scene.editor().buffer().caret());
		}
		CHECK_EQUAL(scene.area.text().size(), forward.back());
		for (size_t index = 1; index < forward.size(); index++)
			CHECK(forward[index] > forward[index - 1]);

		std::vector<size_t> backward = { forward.back() };
		while (backward.back() > 0) {
			driver.key(InputID::LEFT);
			backward.push_back(scene.editor().buffer().caret());
		}
		CHECK_EQUAL(forward.size(), backward.size());
	}

	TEST(RandomResizesKeepScrollInRangeAndCaretVisible) {
		std::string text;
		for (int line = 0; line < 40; line++)
			text += "line " + std::to_string(line) + "\n";
		AreaScene scene(text, 180);
		scene.area.setHeightRange(40, 500);
		UiDriver driver(scene.root, { 800, 900 });
		scene.focus(driver);

		std::mt19937 random(99);
		for (int step = 0; step < 60; step++) {
			scene.area.setBoxHeight(halnf(40 + random() % 460));
			if (random() % 2) driver.key(InputID::PAGE_DOWN);
			else driver.key(InputID::PAGE_UP);
			driver.settle();

			ScrollView& scroll = scene.area.scroll();
			CHECK(scroll.offset() >= -0.5f);
			CHECK(scroll.offset() <= scroll.maximumOffset() + 0.5f);
			const RectF caret = scene.editor().caretRect();
			CHECK(caret.pos.y >= scroll.offset() - 0.5f);
			CHECK(caret.pos.y + caret.size.y <= scroll.offset() + scroll.size().y + 0.5f);
		}
	}
}

SUITE(ColorPickerButton) {
	struct PickerScene {
		Widget root;
		ColorPickerButton picker;
		std::vector<RGBA> picked;
		std::vector<std::vector<RGBA>> palettes;

		PickerScene() {
			root.emplaceLayout<ColumnLayout>();
			picker.setPalette({ RGBA(1, 0, 0, 1), RGBA(0, 1, 0, 1) });
			picker.setOnPicked([this](const RGBA& color) { picked.push_back(color); });
			picker.setOnPaletteChanged([this](const std::vector<RGBA>& colors) { palettes.push_back(colors); });
			root.addChild(picker);
		}
	};

	TEST(PopupGridEndsWithAddCellAndHasNoAutomaticOption) {
		PickerScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.picker);
		driver.settle();

		const std::vector<Widget*>& cells = scene.picker.grid().children();
		REQUIRE CHECK_EQUAL(3u, cells.size());
		CHECK(cells.back() == &scene.picker.addCell());
		CHECK(driver.paint().findText("Automatic") == nullptr);
	}

	TEST(CellsKeepFixedSizeAndGridPositionsWhileOpening) {
		PickerScene scene;
		scene.picker.setPalette(
			{ RGBA(1, 0, 0, 1), RGBA(0, 1, 0, 1), RGBA(0, 0, 1, 1), RGBA(1, 1, 0, 1), RGBA(0, 1, 1, 1) }
		);
		UiDriver driver(scene.root);
		driver.setAutoSettle(false);

		driver.click(scene.picker);
		std::vector<Vec2F> firstFrame;
		for (Widget* cell : scene.picker.grid().children()) {
			CHECK_CLOSE(ColorCell::kSize, cell->size().x, 0.5f);
			CHECK_CLOSE(ColorCell::kSize, cell->size().y, 0.5f);
			firstFrame.push_back(cell->rect().pos);
		}

		driver.settle();
		const std::vector<Widget*>& cells = scene.picker.grid().children();
		for (size_t index = 0; index < cells.size(); index++) {
			CHECK_CLOSE(firstFrame[index].x, cells[index]->rect().pos.x, 0.5f);
			CHECK_CLOSE(firstFrame[index].y, cells[index]->rect().pos.y, 0.5f);
		}

		CHECK_CLOSE(cells[0]->rect().pos.y, cells[3]->rect().pos.y, 0.5f);
		CHECK(cells[4]->rect().pos.y > cells[3]->rect().pos.y);
		CHECK(cells.back() == &scene.picker.addCell());
	}

	TEST(ClickingSwatchPicksAndClosesPopup) {
		PickerScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.picker);
		driver.settle();

		driver.click(scene.picker.swatch(1));
		driver.settle();
		REQUIRE CHECK_EQUAL(1u, scene.picked.size());
		CHECK(scene.picked.back() == RGBA(0, 1, 0, 1));
		CHECK(!scene.picker.isPopupOpen());
		CHECK(scene.picker.swatch(1).isSelected());
	}

	TEST(AddingExistingColorDoesNotDuplicateSwatch) {
		PickerScene scene;
		UiDriver driver(scene.root);
		scene.picker.setColor(RGBA(1, 0, 0, 1));

		driver.click(scene.picker);
		driver.settle();
		driver.click(scene.picker.addCell());
		driver.settle();
		driver.click(scene.picker.confirmNewColorButton());
		driver.settle();

		CHECK_EQUAL(2u, scene.picker.palette().size());
		CHECK_EQUAL(0u, scene.palettes.size());
		REQUIRE CHECK_EQUAL(1u, scene.picked.size());
		CHECK(scene.picker.swatch(0).isSelected());
	}

	TEST(NewColorIsAppendedSelectedAndReported) {
		PickerScene scene;
		UiDriver driver(scene.root);

		driver.click(scene.picker);
		driver.settle();
		driver.click(scene.picker.addCell());
		driver.settle();
		scene.picker.newColorWheel().setColor(RGBA(0.2f, 0.4f, 0.9f, 1));
		const RGBA chosen = scene.picker.newColorWheel().color();
		driver.click(scene.picker.confirmNewColorButton());
		driver.settle();

		REQUIRE CHECK_EQUAL(3u, scene.picker.palette().size());
		CHECK(scene.picker.palette().back() == chosen);
		CHECK(scene.picker.swatch(2).isSelected());
		CHECK(scene.picker.grid().children().back() == &scene.picker.addCell());
		REQUIRE CHECK_EQUAL(1u, scene.palettes.size());
		CHECK_EQUAL(3u, scene.palettes.back().size());
		CHECK(scene.picker.shownColor() == chosen);
	}

	TEST(EscapeClosesNewColorEditorWithoutAdding) {
		PickerScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.picker);
		driver.settle();
		driver.click(scene.picker.addCell());
		driver.settle();

		driver.key(InputID::ESCAPE);
		driver.settle();
		CHECK(!scene.picker.isNewColorEditorOpen());
		CHECK_EQUAL(2u, scene.picker.palette().size());
		CHECK_EQUAL(0u, scene.picked.size());
	}
}
