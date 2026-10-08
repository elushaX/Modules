#pragma once

#include "InputCodes.hpp"
#include "PointerSample.hpp"
#include "TouchGesture.hpp"
#include "Vec.hpp"

#include <string>

namespace tp {

	class WindowInputObserver {
	public:
		virtual ~WindowInputObserver() = default;

		virtual void onPointerSample(const PointerSample& sample) {}
		virtual void onPointerMoved(Vec2F contentPosition) {}
		virtual void onPointerLeft() {}
		virtual void onPointerButton(InputID button, bool pressed) {}
		virtual void onScrolled(Vec2F delta) {}
		virtual void onKey(InputID key, bool pressed) {}
		virtual void onKeyRepeat(InputID key) {}
		virtual void onText(const std::string& utf8) {}
		virtual void onKeyboardFocus(bool focused) {}
		virtual void onGesture(const TouchGesture& gesture) {}
	};
}
