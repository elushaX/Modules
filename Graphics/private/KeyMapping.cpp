#include "KeyMapping.hpp"

#include <linux/input-event-codes.h>

#include <unordered_map>

using namespace tp;

namespace {
	const std::unordered_map<uint32_t, InputID>& evdevKeys() {
		static const std::unordered_map<uint32_t, InputID> keys = {
			{ KEY_SPACE, InputID::SPACE },
			{ KEY_APOSTROPHE, InputID::APOSTROPHE },
			{ KEY_COMMA, InputID::COMMA },
			{ KEY_MINUS, InputID::MINUS },
			{ KEY_DOT, InputID::PERIOD },
			{ KEY_SLASH, InputID::SLASH },
			{ KEY_0, InputID::N0 },
			{ KEY_1, InputID::N1 },
			{ KEY_2, InputID::N2 },
			{ KEY_3, InputID::N3 },
			{ KEY_4, InputID::N4 },
			{ KEY_5, InputID::N5 },
			{ KEY_6, InputID::N6 },
			{ KEY_7, InputID::N7 },
			{ KEY_8, InputID::N8 },
			{ KEY_9, InputID::N9 },
			{ KEY_SEMICOLON, InputID::SEMICOLON },
			{ KEY_EQUAL, InputID::EQUAL },
			{ KEY_A, InputID::A },
			{ KEY_B, InputID::B },
			{ KEY_C, InputID::C },
			{ KEY_D, InputID::D },
			{ KEY_E, InputID::E },
			{ KEY_F, InputID::F },
			{ KEY_G, InputID::G },
			{ KEY_H, InputID::H },
			{ KEY_I, InputID::I },
			{ KEY_J, InputID::J },
			{ KEY_K, InputID::K },
			{ KEY_L, InputID::L },
			{ KEY_M, InputID::M },
			{ KEY_N, InputID::N },
			{ KEY_O, InputID::O },
			{ KEY_P, InputID::P },
			{ KEY_Q, InputID::Q },
			{ KEY_R, InputID::R },
			{ KEY_S, InputID::S },
			{ KEY_T, InputID::T },
			{ KEY_U, InputID::U },
			{ KEY_V, InputID::V },
			{ KEY_W, InputID::W },
			{ KEY_X, InputID::X },
			{ KEY_Y, InputID::Y },
			{ KEY_Z, InputID::Z },
			{ KEY_LEFTBRACE, InputID::LEFT_BRACKET },
			{ KEY_BACKSLASH, InputID::BACKSLASH },
			{ KEY_RIGHTBRACE, InputID::RIGHT_BRACKET },
			{ KEY_GRAVE, InputID::GRAVE_ACCENT },
			{ KEY_102ND, InputID::WORLD_2 },
			{ KEY_ESC, InputID::ESCAPE },
			{ KEY_ENTER, InputID::ENTER },
			{ KEY_TAB, InputID::TAB },
			{ KEY_BACKSPACE, InputID::BACKSPACE },
			{ KEY_INSERT, InputID::INSERT },
			{ KEY_DELETE, InputID::DELETE_KEY },
			{ KEY_RIGHT, InputID::RIGHT },
			{ KEY_LEFT, InputID::LEFT },
			{ KEY_DOWN, InputID::DOWN },
			{ KEY_UP, InputID::UP },
			{ KEY_PAGEUP, InputID::PAGE_UP },
			{ KEY_PAGEDOWN, InputID::PAGE_DOWN },
			{ KEY_HOME, InputID::HOME },
			{ KEY_END, InputID::END },
			{ KEY_CAPSLOCK, InputID::CAPS_LOCK },
			{ KEY_SCROLLLOCK, InputID::SCROLL_LOCK },
			{ KEY_NUMLOCK, InputID::NUM_LOCK },
			{ KEY_SYSRQ, InputID::PRINT_SCREEN },
			{ KEY_PAUSE, InputID::PAUSE },
			{ KEY_F1, InputID::F1 },
			{ KEY_F2, InputID::F2 },
			{ KEY_F3, InputID::F3 },
			{ KEY_F4, InputID::F4 },
			{ KEY_F5, InputID::F5 },
			{ KEY_F6, InputID::F6 },
			{ KEY_F7, InputID::F7 },
			{ KEY_F8, InputID::F8 },
			{ KEY_F9, InputID::F9 },
			{ KEY_F10, InputID::F10 },
			{ KEY_F11, InputID::F11 },
			{ KEY_F12, InputID::F12 },
			{ KEY_F13, InputID::F13 },
			{ KEY_F14, InputID::F14 },
			{ KEY_F15, InputID::F15 },
			{ KEY_F16, InputID::F16 },
			{ KEY_F17, InputID::F17 },
			{ KEY_F18, InputID::F18 },
			{ KEY_F19, InputID::F19 },
			{ KEY_F20, InputID::F20 },
			{ KEY_F21, InputID::F21 },
			{ KEY_F22, InputID::F22 },
			{ KEY_F23, InputID::F23 },
			{ KEY_F24, InputID::F24 },
			{ KEY_KP0, InputID::KP_0 },
			{ KEY_KP1, InputID::KP_1 },
			{ KEY_KP2, InputID::KP_2 },
			{ KEY_KP3, InputID::KP_3 },
			{ KEY_KP4, InputID::KP_4 },
			{ KEY_KP5, InputID::KP_5 },
			{ KEY_KP6, InputID::KP_6 },
			{ KEY_KP7, InputID::KP_7 },
			{ KEY_KP8, InputID::KP_8 },
			{ KEY_KP9, InputID::KP_9 },
			{ KEY_KPDOT, InputID::KP_DECIMAL },
			{ KEY_KPSLASH, InputID::KP_DIVIDE },
			{ KEY_KPASTERISK, InputID::KP_MULTIPLY },
			{ KEY_KPMINUS, InputID::KP_SUBTRACT },
			{ KEY_KPPLUS, InputID::KP_ADD },
			{ KEY_KPENTER, InputID::KP_ENTER },
			{ KEY_KPEQUAL, InputID::KP_EQUAL },
			{ KEY_LEFTSHIFT, InputID::LEFT_SHIFT },
			{ KEY_LEFTCTRL, InputID::LEFT_CONTROL },
			{ KEY_LEFTALT, InputID::LEFT_ALT },
			{ KEY_LEFTMETA, InputID::LEFT_SUPER },
			{ KEY_RIGHTSHIFT, InputID::RIGHT_SHIFT },
			{ KEY_RIGHTCTRL, InputID::RIGHT_CONTROL },
			{ KEY_RIGHTALT, InputID::RIGHT_ALT },
			{ KEY_RIGHTMETA, InputID::RIGHT_SUPER },
			{ KEY_COMPOSE, InputID::MENU },
		};
		return keys;
	}
}

std::optional<InputID> tp::inputFromEvdevKey(uint32_t evdevKey) {
	const auto& keys = evdevKeys();
	const auto found = keys.find(evdevKey);
	if (found == keys.end()) return std::nullopt;
	return found->second;
}

std::optional<InputID> tp::inputFromPointerButton(uint32_t linuxButton) {
	switch (linuxButton) {
		case BTN_LEFT: return InputID::MOUSE1;
		case BTN_RIGHT: return InputID::MOUSE2;
		case BTN_MIDDLE: return InputID::MOUSE3;
		case BTN_SIDE: return InputID::MOUSE4;
		case BTN_EXTRA: return InputID::MOUSE5;
		default: return std::nullopt;
	}
}
