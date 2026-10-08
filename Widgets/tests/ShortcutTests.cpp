#include "Shortcut.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;

namespace {
	enum class TestCommand { Undo, Redo, Zoom };

	Event keyDown(InputID key, Modifiers modifiers = {}) {
		return Event::key(Event::Type::KeyDown, key).withModifiers(modifiers);
	}

	Modifiers control(bool shift = false) {
		Modifiers modifiers;
		modifiers.control = true;
		modifiers.shift = shift;
		return modifiers;
	}
}

SUITE(Shortcuts) {
	TEST(MatchingRequiresExactModifiers) {
		CHECK(ctrl(InputID::Z).matches(keyDown(InputID::Z, control())));
		CHECK(!ctrl(InputID::Z).matches(keyDown(InputID::Z)));
		CHECK(!ctrl(InputID::Z).matches(keyDown(InputID::Z, control(true))));
		CHECK(ctrlShift(InputID::Z).matches(keyDown(InputID::Z, control(true))));
	}

	TEST(TextListsModifiersInFixedOrder) {
		CHECK_EQUAL("Ctrl+Shift+Z", ctrlShift(InputID::Z).text());
		CHECK_EQUAL("Ctrl+Alt+A", ctrlAlt(InputID::A).text());
		CHECK_EQUAL("Alt+`", alt(InputID::GRAVE_ACCENT).text());
		CHECK_EQUAL("F11", plain(InputID::F11).text());
		CHECK_EQUAL("Ctrl+Num0", ctrl(InputID::KP_0).text());
		CHECK_EQUAL("Delete", plain(InputID::DELETE_KEY).text());
		CHECK_EQUAL("[", plain(InputID::LEFT_BRACKET).text());
	}

	TEST(MapFindsCommandForKeyDownOnly) {
		ShortcutMap<TestCommand> shortcuts;
		shortcuts.bind(ctrl(InputID::Z), TestCommand::Undo);

		const auto* binding = shortcuts.find(keyDown(InputID::Z, control()));
		CHECK(binding != nullptr);
		CHECK(binding->command == TestCommand::Undo);
		CHECK(shortcuts.find(Event::key(Event::Type::KeyUp, InputID::Z).withModifiers(control())) == nullptr);
		CHECK(shortcuts.find(keyDown(InputID::Y, control())) == nullptr);
	}

	TEST(RepeatOnlyTriggersRepeatingBindings) {
		ShortcutMap<TestCommand> shortcuts;
		shortcuts.bind(ctrl(InputID::Z), TestCommand::Undo, true);
		shortcuts.bind(plain(InputID::Z), TestCommand::Zoom);

		CHECK(shortcuts.find(Event::keyRepeat(InputID::Z).withModifiers(control())) != nullptr);
		CHECK(shortcuts.find(Event::keyRepeat(InputID::Z)) == nullptr);
	}

	TEST(RebindingAShortcutReplacesItsCommand) {
		ShortcutMap<TestCommand> shortcuts;
		shortcuts.bind(ctrl(InputID::Y), TestCommand::Undo);
		shortcuts.bind(ctrl(InputID::Y), TestCommand::Redo);

		CHECK_EQUAL(1u, shortcuts.bindings().size());
		CHECK(shortcuts.find(keyDown(InputID::Y, control()))->command == TestCommand::Redo);
	}

	TEST(CommandsCanHaveSeveralShortcutsAndReportTheFirst) {
		ShortcutMap<TestCommand> shortcuts;
		shortcuts.bind(ctrl(InputID::Z), TestCommand::Undo);
		shortcuts.bind(ctrl(InputID::Y), TestCommand::Undo);

		CHECK_EQUAL(2u, shortcuts.shortcutsFor(TestCommand::Undo).size());
		CHECK_EQUAL("Ctrl+Z", shortcuts.textFor(TestCommand::Undo));
		CHECK_EQUAL("", shortcuts.textFor(TestCommand::Redo));

		shortcuts.unbind(ctrl(InputID::Z));
		CHECK_EQUAL("Ctrl+Y", shortcuts.textFor(TestCommand::Undo));
	}
}
