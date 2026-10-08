#pragma once

#include "InputCodes.hpp"
#include "Rect.hpp"
#include "WindowTypes.hpp"

#include <cstdint>
#include <memory>
#include <string>

struct wl_surface;

namespace tp {

	class WaylandConnection;
	class WaylandWindow;
	class WindowInputObserver;

	class WaylandSeat {
	public:
		WaylandSeat(WaylandConnection& connection, WaylandWindow& window);
		~WaylandSeat();

		WaylandSeat(const WaylandSeat&) = delete;
		WaylandSeat& operator=(const WaylandSeat&) = delete;

		[[nodiscard]] int repeatTimerDescriptor() const;
		void onRepeatTimer();

		[[nodiscard]] uint32_t lastInputSerial() const { return mLastInputSerial; }

		void setCursor(CursorShape shape);

		void setClipboardText(const std::string& text);
		[[nodiscard]] std::string clipboardText();
		[[nodiscard]] ClipboardImage clipboardImage();

		void setTextInputEnabled(bool enabled);
		void setTextInputRect(const RectF& surfaceRect);

		[[nodiscard]] bool hasPointer() const { return mPointer != nullptr; }
		[[nodiscard]] bool hasKeyboard() const { return mKeyboard != nullptr; }
		[[nodiscard]] bool hasTouch() const { return mHasTouch; }
		[[nodiscard]] bool hasTablet() const { return mTablet != nullptr; }

		class Keyboard;
		class Pointer;
		class Clipboard;
		class TextInput;
		class Tablet;

	private:
		friend struct WaylandSeatListeners;

		void onCapabilities(uint32_t capabilities);
		void recordSerial(uint32_t serial) { mLastInputSerial = serial; }

		WaylandConnection& mConnection;
		WaylandWindow& mWindow;

		[[nodiscard]] WindowInputObserver& input();

		std::unique_ptr<Keyboard> mKeyboard;
		std::unique_ptr<Pointer> mPointer;
		std::unique_ptr<Clipboard> mClipboard;
		std::unique_ptr<TextInput> mTextInput;
		std::unique_ptr<Tablet> mTablet;

		uint32_t mLastInputSerial = 0;
		bool mHasTouch = false;
	};
}
