#include "Shortcut.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	Shortcut make(InputID key, bool control, bool shift, bool alt) {
		Shortcut shortcut;
		shortcut.key = key;
		shortcut.modifiers.control = control;
		shortcut.modifiers.shift = shift;
		shortcut.modifiers.alt = alt;
		return shortcut;
	}

	bool sameModifiers(const Modifiers& left, const Modifiers& right) {
		return left.control == right.control && left.shift == right.shift && left.alt == right.alt;
	}
}

bool Shortcut::matches(const Event& event) const {
	return event.keyCode() == key && sameModifiers(event.modifiers(), modifiers);
}

bool Shortcut::operator==(const Shortcut& other) const {
	return key == other.key && sameModifiers(modifiers, other.modifiers);
}

std::string Shortcut::text() const {
	std::string text;
	if (modifiers.control) text += "Ctrl+";
	if (modifiers.shift) text += "Shift+";
	if (modifiers.alt) text += "Alt+";
	return text + keyName(key);
}

Shortcut tp::ui::ctrl(InputID key) { return make(key, true, false, false); }
Shortcut tp::ui::ctrlShift(InputID key) { return make(key, true, true, false); }
Shortcut tp::ui::ctrlAlt(InputID key) { return make(key, true, false, true); }
Shortcut tp::ui::shift(InputID key) { return make(key, false, true, false); }
Shortcut tp::ui::alt(InputID key) { return make(key, false, false, true); }
Shortcut tp::ui::plain(InputID key) { return make(key, false, false, false); }

std::string tp::ui::keyName(InputID key) {
	const auto code = (ualni) key;
	if (code >= (ualni) InputID::A && code <= (ualni) InputID::Z) return std::string(1, (char) code);
	if (code >= (ualni) InputID::N0 && code <= (ualni) InputID::N9) return std::string(1, (char) code);
	if (code >= (ualni) InputID::F1 && code <= (ualni) InputID::F25) {
		return "F" + std::to_string(code - (ualni) InputID::F1 + 1);
	}
	if (code >= (ualni) InputID::KP_0 && code <= (ualni) InputID::KP_9) {
		return "Num" + std::to_string(code - (ualni) InputID::KP_0);
	}

	switch (key) {
		case InputID::SPACE: return "Space";
		case InputID::APOSTROPHE: return "'";
		case InputID::COMMA: return ",";
		case InputID::MINUS: return "-";
		case InputID::PERIOD: return ".";
		case InputID::SLASH: return "/";
		case InputID::SEMICOLON: return ";";
		case InputID::EQUAL: return "=";
		case InputID::LEFT_BRACKET: return "[";
		case InputID::BACKSLASH: return "\\";
		case InputID::RIGHT_BRACKET: return "]";
		case InputID::GRAVE_ACCENT: return "`";
		case InputID::ESCAPE: return "Esc";
		case InputID::ENTER: return "Enter";
		case InputID::TAB: return "Tab";
		case InputID::BACKSPACE: return "Back";
		case InputID::INSERT: return "Insert";
		case InputID::DELETE_KEY: return "Delete";
		case InputID::RIGHT: return "Right";
		case InputID::LEFT: return "Left";
		case InputID::DOWN: return "Down";
		case InputID::UP: return "Up";
		case InputID::PAGE_UP: return "PgUp";
		case InputID::PAGE_DOWN: return "PgDn";
		case InputID::HOME: return "Home";
		case InputID::END: return "End";
		case InputID::KP_DECIMAL: return "Num,";
		case InputID::KP_DIVIDE: return "Num/";
		case InputID::KP_MULTIPLY: return "Num*";
		case InputID::KP_SUBTRACT: return "Num-";
		case InputID::KP_ADD: return "Num+";
		case InputID::KP_ENTER: return "NumEnter";
		default: return "?";
	}
}
