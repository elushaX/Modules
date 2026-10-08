#pragma once

#include "AppStorage.hpp"
#include "Geometry.hpp"
#include "PlatformInfo.hpp"
#include "WindowTypes.hpp"

#include <string>

namespace tp::ui {

	class WindowHost {
	public:
		virtual ~WindowHost() = default;

		[[nodiscard]] virtual PlatformInfo platformInfo() const = 0;

		virtual void minimizeWindow() = 0;
		virtual void toggleMaximizeWindow() = 0;
		virtual void closeWindow() = 0;
		[[nodiscard]] virtual bool isWindowMaximized() const = 0;

		virtual void beginWindowMove() = 0;
		virtual void showWindowMenu(Vec2F windowPoint) = 0;

		virtual void setCursor(CursorShape shape) = 0;

		virtual void setClipboardText(const std::string& text) = 0;
		[[nodiscard]] virtual std::string clipboardText() = 0;
		[[nodiscard]] virtual ClipboardImage clipboardImage() { return {}; }
		virtual bool openUrl(const std::string& url) = 0;
		virtual void setTextInputArea(const RectF* windowRect) = 0;

		[[nodiscard]] virtual AppStorage persistentAppStorage() const = 0;
	};
}
