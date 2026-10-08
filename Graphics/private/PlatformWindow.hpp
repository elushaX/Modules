#pragma once

#include "InputCoalescer.hpp"
#include "PlatformInfo.hpp"
#include "Rect.hpp"
#include "VulkanContext.hpp"
#include "WindowFrame.hpp"
#include "WindowTypes.hpp"

#include <memory>
#include <string>

namespace tp {

	class Window;

	class PlatformWindow : public VulkanSurfaceSource {
	public:
		~PlatformWindow() override = default;

		virtual void setAcceptsInput(bool accepts) = 0;

		virtual void dispatchEvents(halnf timeoutMs) = 0;
		virtual void wake() = 0;
		virtual void waitUntilConfigured() = 0;
		virtual void commitFrameGeometry() = 0;
		virtual void flushPendingInput() = 0;
		[[nodiscard]] virtual bool isWaitingForFrame() const = 0;

		[[nodiscard]] virtual bool shouldClose() const = 0;
		virtual void requestClose() = 0;

		[[nodiscard]] virtual const WindowFrame& frame() const = 0;
		[[nodiscard]] virtual Vec2F contentSize() const = 0;
		[[nodiscard]] virtual halnf scale() const = 0;
		[[nodiscard]] virtual halnf refreshIntervalMs() const = 0;
		[[nodiscard]] virtual bool isMaximized() const = 0;
		[[nodiscard]] virtual bool isFocused() const = 0;

		virtual void minimize() = 0;
		virtual void toggleMaximize() = 0;
		virtual void setFullScreen(bool fullScreen) = 0;
		[[nodiscard]] virtual bool isFullScreen() const = 0;
		virtual void beginMove() = 0;
		virtual void beginResize(WindowEdge edge) = 0;
		virtual void showWindowMenu(Vec2F contentPosition) = 0;

		virtual void setCursor(CursorShape shape) = 0;
		virtual void setClipboardText(const std::string& text) = 0;
		[[nodiscard]] virtual std::string clipboardText() = 0;
		[[nodiscard]] virtual ClipboardImage clipboardImage() = 0;
		virtual bool openUrl(const std::string& url) = 0;
		virtual void setTextInputEnabled(bool enabled) = 0;
		virtual void setTextInputRect(const RectF& contentRect) = 0;

		[[nodiscard]] virtual PlatformInfo platformInfo() const = 0;

		[[nodiscard]] virtual InputCoalescer& coalescer() = 0;
		[[nodiscard]] virtual const InputCoalescer& coalescer() const = 0;
	};

	[[nodiscard]] std::unique_ptr<PlatformWindow>
	createPlatformWindow(Window& owner, Vec2F contentSize, const WindowIdentity& identity);
}
