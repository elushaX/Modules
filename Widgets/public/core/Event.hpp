#pragma once

#include "Geometry.hpp"
#include "InputCodes.hpp"
#include "PointerSample.hpp"
#include "TouchGesture.hpp"

#include <string>

namespace tp::ui {

	class Widget;

	enum class PointerButton { Primary, Secondary, Middle };

	struct Modifiers {
		bool shift = false;
		bool control = false;
		bool alt = false;
	};

	class Event {
	public:
		enum class Type {
			PointerMove,
			PointerDown,
			PointerUp,
			Click,
			LongPress,
			Wheel,
			KeyDown,
			KeyUp,
			TextInput,
			Gesture
		};

		static Event pointer(Type type, Vec2F windowPosition, Vec2F delta, PointerButton button);
		static Event wheel(Vec2F windowPosition, halnf amount, halnf horizontalAmount = 0);
		static Event key(Type type, InputID key);
		static Event keyRepeat(InputID key);
		static Event text(std::string utf8);
		static Event gesture(const TouchGesture& gesture);

		Event& withModifiers(const Modifiers& modifiers);
		Event& withPressure(halnf pressure);
		Event& withSample(const PointerSample& sample);
		Event& withButtonsDown(bool primary, bool secondary, bool middle);
		Event& withClickCount(int clickCount);

		[[nodiscard]] Type type() const { return mType; }
		[[nodiscard]] bool isPointerEvent() const;
		[[nodiscard]] bool isKeyEvent() const {
			return mType == Type::KeyDown || mType == Type::KeyUp || mType == Type::TextInput;
		}

		[[nodiscard]] Vec2F position() const;
		[[nodiscard]] Vec2F windowPosition() const { return mWindowPosition; }
		[[nodiscard]] Vec2F delta() const { return mDelta; }
		[[nodiscard]] PointerButton button() const { return mButton; }
		[[nodiscard]] bool isDown(PointerButton button) const;
		[[nodiscard]] halnf pressure() const { return mPressure; }
		[[nodiscard]] const PointerSample& sample() const { return mSample; }
		[[nodiscard]] int clickCount() const { return mClickCount; }
		[[nodiscard]] halnf wheelAmount() const { return mWheelAmount; }
		[[nodiscard]] halnf horizontalWheelAmount() const { return mHorizontalWheelAmount; }
		[[nodiscard]] InputID keyCode() const { return mKey; }
		[[nodiscard]] bool isRepeat() const { return mRepeat; }
		[[nodiscard]] const std::string& text() const { return mText; }
		[[nodiscard]] const Modifiers& modifiers() const { return mModifiers; }
		[[nodiscard]] const TouchGesture& touchGesture() const { return mGesture; }

		[[nodiscard]] Widget* target() const { return mTarget; }
		[[nodiscard]] Widget* receiver() const { return mReceiver; }

		void accept() { mAccepted = true; }
		[[nodiscard]] bool isAccepted() const { return mAccepted; }

	private:
		friend class InputRouter;

		explicit Event(Type type) :
			mType(type) {}

		Type mType;
		Vec2F mWindowPosition = { 0, 0 };
		Vec2F mDelta = { 0, 0 };
		PointerButton mButton = PointerButton::Primary;
		bool mButtonsDown[3] = { false, false, false };
		halnf mPressure = 0;
		PointerSample mSample;
		int mClickCount = 1;
		halnf mWheelAmount = 0;
		halnf mHorizontalWheelAmount = 0;
		InputID mKey = InputID::SPACE;
		bool mRepeat = false;
		std::string mText;
		Modifiers mModifiers;
		TouchGesture mGesture;

		Widget* mTarget = nullptr;
		Widget* mReceiver = nullptr;
		bool mAccepted = false;
	};
}
