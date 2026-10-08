#include "BoxLayout.hpp"
#include "FakeWindowHost.hpp"
#include "TextField.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct FieldScene {
		Widget root;
		TextField field;
		TextField other;

		FieldScene() {
			root.emplaceLayout<ColumnLayout>(10);
			root.addChild(field);
			root.addChild(other);
		}
	};

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
}

SUITE(TextFieldEditing) {
	TEST(ClickFocusesAndSelectsAllThenTypingReplaces) {
		FieldScene scene;
		scene.field.setText("hello");
		UiDriver driver(scene.root);

		driver.click(scene.field);
		CHECK(scene.field.isEditing());
		CHECK_EQUAL("hello", scene.field.selectedText());

		driver.type("bye");
		CHECK_EQUAL("bye", scene.field.text());
		CHECK_EQUAL(3u, scene.field.caret());
	}

	TEST(CaretMovementAndEditingKeys) {
		FieldScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.field);
		driver.type("abcd");

		driver.key(InputID::LEFT);
		driver.key(InputID::LEFT);
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("acd", scene.field.text());

		driver.key(InputID::DELETE_KEY);
		CHECK_EQUAL("ad", scene.field.text());

		driver.key(InputID::HOME);
		driver.type("x");
		driver.key(InputID::END);
		driver.type("y");
		CHECK_EQUAL("xady", scene.field.text());
	}

	TEST(ShiftExtendsSelectionAndWordJumps) {
		FieldScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.field);
		driver.type("one two three");

		driver.key(InputID::LEFT, control(true));
		CHECK_EQUAL("three", scene.field.selectedText());

		driver.key(InputID::LEFT, shift());
		CHECK_EQUAL(" three", scene.field.selectedText());

		driver.key(InputID::BACKSPACE, control());
		CHECK_EQUAL("one two", scene.field.text());

		driver.key(InputID::BACKSPACE, control());
		CHECK_EQUAL("one ", scene.field.text());
	}

	TEST(MultibyteCharactersMoveAsOneUnit) {
		FieldScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.field);
		driver.type("añb");

		driver.key(InputID::LEFT);
		driver.key(InputID::BACKSPACE);
		CHECK_EQUAL("ab", scene.field.text());
	}

	TEST(FilterAndMaximumLengthLimitInput) {
		FieldScene scene;
		scene.field.setMaximumLength(4);
		scene.field.setCharacterFilter([](const std::string& character) { return character != "x"; });
		UiDriver driver(scene.root);
		driver.click(scene.field);

		driver.type("axbxcdef");
		CHECK_EQUAL("abcd", scene.field.text());
	}

	TEST(EnterCommitsAndEditCallbacksFire) {
		FieldScene scene;
		std::string committed;
		int edits = 0;
		scene.field.setOnCommitted([&committed](const std::string& text) { committed = text; });
		scene.field.setOnEdited([&edits](const std::string&) { edits++; });
		UiDriver driver(scene.root);
		driver.click(scene.field);

		driver.type("o");
		driver.type("k");
		CHECK_EQUAL(2, edits);
		driver.key(InputID::ENTER);
		CHECK_EQUAL("ok", committed);
	}

	TEST(LosingFocusCommits) {
		FieldScene scene;
		std::string committed;
		scene.field.setOnCommitted([&committed](const std::string& text) { committed = text; });
		UiDriver driver(scene.root);
		driver.click(scene.field);
		driver.type("left");

		driver.click(scene.other);
		driver.advance(16);
		CHECK_EQUAL("left", committed);
		CHECK(!scene.field.isEditing());
	}

	TEST(ClipboardCopyCutPasteUseWindowHost) {
		FieldScene scene;
		FakeWindowHost host;
		UiDriver driver(scene.root);
		driver.ui().setWindowHost(&host);
		driver.click(scene.field);
		driver.type("copy me");

		driver.key(InputID::A, control());
		driver.key(InputID::C, control());
		CHECK_EQUAL("copy me", host.clipboard);

		driver.key(InputID::X, control());
		CHECK_EQUAL("", scene.field.text());

		host.clipboard = "pasted";
		driver.key(InputID::V, control());
		CHECK_EQUAL("pasted", scene.field.text());
		driver.ui().setWindowHost(nullptr);
	}

	TEST(FocusReportsTextInputAreaToHost) {
		FieldScene scene;
		FakeWindowHost host;
		UiDriver driver(scene.root);
		driver.ui().setWindowHost(&host);

		driver.click(scene.field);
		CHECK(host.textInputEnabled);
		CHECK(host.textInputRect == scene.field.windowRect());

		driver.click(scene.other);
		driver.advance(16);
		CHECK(host.textInputEnabled);
		CHECK(host.textInputRect == scene.other.windowRect());
		driver.ui().setWindowHost(nullptr);
	}

	TEST(RemovingAFocusedFieldReleasesTextInput) {
		FieldScene scene;
		FakeWindowHost host;
		UiDriver driver(scene.root);
		driver.ui().setWindowHost(&host);
		driver.click(scene.field);
		REQUIRE CHECK(host.textInputEnabled);

		scene.root.removeChild(scene.field);
		driver.advance(16);

		CHECK(!host.textInputEnabled);
		driver.ui().setWindowHost(nullptr);
	}

	TEST(FocusedFieldDoesNotSwallowShortcuts) {
		FieldScene scene;
		UiDriver driver(scene.root);
		driver.click(scene.field);

		Event undo = Event::key(Event::Type::KeyDown, InputID::Z).withModifiers(control());
		driver.ui().dispatch(undo);
		CHECK_EQUAL("", scene.field.text());
	}
}

SUITE(NumberFieldStepping) {
	TEST(StepButtonsDrawVisibleMinusAndPlusIcons) {
		NumberField number(2, 1000, 30);
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		root.addChild(number);
		UiDriver driver(root);
		const RecordingPainter& painter = driver.paint();

		const auto linesInside = [&](const Widget& button) {
			const RectF area = button.windowRect();
			size_t count = 0;
			for (const PaintCommand& command : painter.commands()) {
				if (command.kind != PaintCommand::Kind::Line) continue;
				const Vec2F middle = command.rect.pos + command.rect.size / 2.0f;
				if (
					middle.x >= area.pos.x && middle.x <= area.pos.x + area.size.x && middle.y >= area.pos.y &&
					middle.y <= area.pos.y + area.size.y
				)
					count++;
			}
			return count;
		};
		CHECK_EQUAL(1u, linesInside(number.decrementButton()));
		CHECK_EQUAL(2u, linesInside(number.incrementButton()));
		CHECK(number.decrementButton().windowRect().size.x >= 24);
		CHECK(!painter.hasText("-"));
		CHECK(!painter.hasText("+"));
	}

	TEST(ButtonsStepAndClampToRange) {
		NumberField number(2, 1000, 30);
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		root.addChild(number);
		UiDriver driver(root);
		halnf reported = 0;
		number.setOnChanged([&reported](halnf value) { reported = value; });

		driver.click(number.incrementButton());
		CHECK_CLOSE(31, number.value(), 0.001f);
		CHECK_CLOSE(31, reported, 0.001f);
		CHECK_EQUAL("31", number.field().text());

		number.setValue(2);
		driver.click(number.decrementButton());
		CHECK_CLOSE(2, number.value(), 0.001f);
	}

	TEST(TypedValueIsParsedAndClampedOnCommit) {
		NumberField number(2, 1000, 30);
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		root.addChild(number);
		UiDriver driver(root);

		driver.click(number.field());
		driver.type("5000");
		driver.key(InputID::ENTER);
		CHECK_CLOSE(1000, number.value(), 0.001f);
		CHECK_EQUAL("1000", number.field().text());

		driver.click(number.field());
		driver.type("4,5");
		driver.key(InputID::ENTER);
		CHECK_CLOSE(4.5f, number.value(), 0.001f);
	}

	TEST(ArrowKeysAndWheelStep) {
		NumberField number(0, 100, 10, 5);
		number.setSuffix(" %");
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		root.addChild(number);
		UiDriver driver(root);

		driver.click(number.field());
		driver.key(InputID::UP);
		CHECK_CLOSE(15, number.value(), 0.001f);
		driver.key(InputID::DOWN);
		driver.key(InputID::DOWN);
		CHECK_CLOSE(5, number.value(), 0.001f);

		driver.moveTo(number.field());
		driver.wheel(1);
		CHECK_CLOSE(10, number.value(), 0.001f);
		CHECK_EQUAL("10 %", number.field().text());
	}

	TEST(TextSetBeforeTheFirstLayoutShowsFromItsStart) {
		TextField field;
		field.setText("3CC0FF");
		field.setSizing(AxisSizing::fixed(160), AxisSizing::fit());
		Widget root;
		root.emplaceLayout<ColumnLayout>(0);
		root.addChild(field);
		UiDriver driver(root, { 400, 200 });
		driver.paint();
		const PaintCommand* shown = driver.paint().findText("3CC0FF");
		REQUIRE CHECK(shown != nullptr);
		CHECK(shown->rect.pos.x >= field.windowRect().pos.x);
	}

	TEST(UnfocusedFieldsShowTheStartAfterANarrowFirstPaint) {
		TextField field;
		field.setText("3CC0FF");
		field.setSizing(AxisSizing::fixed(20), AxisSizing::fit());
		Widget root;
		root.emplaceLayout<ColumnLayout>(0);
		root.addChild(field);
		UiDriver driver(root, { 400, 200 });
		driver.paint();
		field.setSizing(AxisSizing::fixed(160), AxisSizing::fit());
		driver.layout();
		const PaintCommand* shown = driver.paint().findText("3CC0FF");
		REQUIRE CHECK(shown != nullptr);
		CHECK(shown->rect.pos.x >= field.windowRect().pos.x);
	}

	TEST(FocusedLongTextStillFollowsTheCaret) {
		TextField field;
		field.setSizing(AxisSizing::fixed(60), AxisSizing::fit());
		Widget root;
		root.emplaceLayout<ColumnLayout>(0);
		root.addChild(field);
		UiDriver driver(root, { 400, 200 });
		driver.click(field);
		driver.type("a much longer text than fits");
		const PaintCommand* shown = driver.paint().findText("a much longer text than fits");
		REQUIRE CHECK(shown != nullptr);
		CHECK(shown->rect.pos.x < field.windowRect().pos.x);
	}
}
