#pragma once

#include "PlatformInfo.hpp"

#include "Buffer.hpp"
#include "EventHandler.hpp"
#include "WindowFrame.hpp"
#include "InputCoalescer.hpp"
#include "WindowInput.hpp"
#include "WindowTypes.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tp {
	class VulkanContext;
	class WaylandWindow;
	class AndroidWindow;
	class WindowInputDispatcher;

	class Window {
	public:
		class Context;

	private:
		Window(Vec2F size, const WindowIdentity& identity);
		~Window();

	public:
		static Window* createWindow(Vec2F size, const WindowIdentity& identity);
		static void destroyWindow(Window* window);

	public:
		void draw();
		void processEvents(bool wait = true);
		void processEventsFor(halnf timeoutMs);
		void wake();
		void flushPendingInput();
		[[nodiscard]] bool isWaitingForFrame() const;
		[[nodiscard]] bool hasPendingWork() const;
		void setInputDeliveryPolicies(const InputDeliveryPolicies& policies);
		[[nodiscard]] const CoalescingStatistics& inputStatistics() const;
		[[nodiscard]] uint64_t swapchainRecreations() const;
		[[nodiscard]] halnf refreshIntervalMs() const;
		void waitForReady();

		bool frameBegin();
		void frameEnd();

		void setEventHandler(EventHandler* eventHandler);
		[[nodiscard]] EventHandler* getEventHandler();

		void addInputObserver(WindowInputObserver* observer);
		void removeInputObserver(WindowInputObserver* observer);
		[[nodiscard]] WindowInputObserver& injectInput();
		void setRawInputTap(WindowInputObserver* tap);
		void setPlatformInputEnabled(bool enabled);
		[[nodiscard]] bool isPlatformInputEnabled() const { return mPlatformInputEnabled; }

		Context* getContext();
		[[nodiscard]] VulkanContext* vulkan();
		[[nodiscard]] bool shouldClose() const;
		void requestClose();
		[[nodiscard]] const Vec2F& getSize() const;
		[[nodiscard]] Vec2F getFramebufferSize() const;
		[[nodiscard]] halnf getContentScale() const;

		[[nodiscard]] const WindowFrame& frame() const;
		[[nodiscard]] PlatformInfo platformInfo() const;

		void minimize();
		void toggleMaximize();
		[[nodiscard]] bool isMaximized() const;
		void setFullScreen(bool fullScreen);
		[[nodiscard]] bool isFullScreen() const;
		[[nodiscard]] bool isFocused() const;
		void beginMove();
		void beginResize(WindowEdge edge);
		void showWindowMenu(Vec2F contentPosition);

		void setCursor(CursorShape shape);

		void setClipboardText(const std::string& text);
		[[nodiscard]] std::string clipboardText();
		[[nodiscard]] ClipboardImage clipboardImage();
		bool openUrl(const std::string& url);

		void setTextInputEnabled(bool enabled);
		void setTextInputRect(const RectF& contentRect);

	private:
		friend class WaylandWindow;
		friend class AndroidWindow;
		friend class WaylandSeat;

		[[nodiscard]] WindowInputObserver& dispatcher();
		void notifyResized();

		Vec2F mSize;
		Context* mContext;
		std::unique_ptr<WindowInputDispatcher> mInput;
		bool mPlatformInputEnabled = true;
	};
}
