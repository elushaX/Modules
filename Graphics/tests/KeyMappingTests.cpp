#include "UnitTest++/UnitTest++.h"

#include "KeyMapping.hpp"

#include <linux/input-event-codes.h>

using namespace tp;

SUITE(EvdevKeyMapping) {
	TEST(LettersAndDigitsKeepTheirInputCodes) {
		CHECK(inputFromEvdevKey(KEY_A) == InputID::A);
		CHECK(inputFromEvdevKey(KEY_Z) == InputID::Z);
		CHECK(inputFromEvdevKey(KEY_0) == InputID::N0);
		CHECK(inputFromEvdevKey(KEY_9) == InputID::N9);
	}

	TEST(NavigationAndEditingKeysAreMapped) {
		CHECK(inputFromEvdevKey(KEY_ESC) == InputID::ESCAPE);
		CHECK(inputFromEvdevKey(KEY_ENTER) == InputID::ENTER);
		CHECK(inputFromEvdevKey(KEY_BACKSPACE) == InputID::BACKSPACE);
		CHECK(inputFromEvdevKey(KEY_LEFT) == InputID::LEFT);
		CHECK(inputFromEvdevKey(KEY_PAGEDOWN) == InputID::PAGE_DOWN);
	}

	TEST(ModifiersKeepSides) {
		CHECK(inputFromEvdevKey(KEY_LEFTCTRL) == InputID::LEFT_CONTROL);
		CHECK(inputFromEvdevKey(KEY_RIGHTSHIFT) == InputID::RIGHT_SHIFT);
		CHECK(inputFromEvdevKey(KEY_LEFTALT) == InputID::LEFT_ALT);
	}

	TEST(UnknownKeysAreIgnored) { CHECK(!inputFromEvdevKey(KEY_MAX).has_value()); }

	TEST(PointerButtonsMapToMouseInputs) {
		CHECK(inputFromPointerButton(BTN_LEFT) == InputID::MOUSE1);
		CHECK(inputFromPointerButton(BTN_RIGHT) == InputID::MOUSE2);
		CHECK(inputFromPointerButton(BTN_MIDDLE) == InputID::MOUSE3);
		CHECK(!inputFromPointerButton(BTN_TOUCH).has_value());
	}
}
