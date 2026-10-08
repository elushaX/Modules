#pragma once

#include "TouchGesture.hpp"
#include "PlatformWindow.hpp"

#include <cstdint>
#include <memory>
#include <string>

struct wl_display;

namespace tp {

	class Window;
	class WaylandConnection;
	class WaylandSeat;
	class WindowInputObserver;

	class WaylandWindow final : public PlatformWindow {
	public:
		WaylandWindow(Window& owner, Vec2F contentSize, const WindowIdentity& identity);
		~WaylandWindow() override;

		WaylandWindow(const WaylandWindow&) = delete;
		WaylandWindow& operator=(const WaylandWindow&) = delete;

		[[nodiscard]] std::vector<const char*> requiredInstanceExtensions() const override;
		[[nodiscard]] VkSurfaceKHR createSurface(VkInstance instance) const override;
		[[nodiscard]] VkExtent2D framebufferExtent() const override;
		[[nodiscard]] bool wantsTranslucency() const override { return true; }

		[[nodiscard]] WindowInputObserver& platformInput();
		void setAcceptsInput(bool accepts) override;
		void onTranslucencyResolved(bool translucent) override;

		void dispatchEvents(halnf timeoutMs) override;
		void wake() override;
		void waitUntilConfigured() override;
		void commitFrameGeometry() override;

		[[nodiscard]] bool shouldClose() const override { return mCloseRequested; }
		void requestClose() override { mCloseRequested = true; }

		[[nodiscard]] const WindowFrame& frame() const override { return mFrame; }
		[[nodiscard]] Vec2F contentSize() const override { return mFrame.contentSize(); }
		[[nodiscard]] halnf scale() const override { return mScale; }
		[[nodiscard]] halnf refreshIntervalMs() const override;
		[[nodiscard]] bool isMaximized() const override { return mMaximized; }
		[[nodiscard]] bool isFocused() const override { return mActivated; }

		void minimize() override;
		void toggleMaximize() override;
		void setFullScreen(bool fullScreen) override;
		[[nodiscard]] bool isFullScreen() const override { return mFullScreen; }
		void beginMove() override;
		void beginResize(WindowEdge edge) override;
		void showWindowMenu(Vec2F contentPosition) override;

		void setCursor(CursorShape shape) override;
		void setClipboardText(const std::string& text) override;
		[[nodiscard]] std::string clipboardText() override;
		[[nodiscard]] ClipboardImage clipboardImage() override;
		bool openUrl(const std::string& url) override;
		void setTextInputEnabled(bool enabled) override;
		void setTextInputRect(const RectF& contentRect) override;

		[[nodiscard]] PlatformInfo platformInfo() const override;

		[[nodiscard]] InputCoalescer& coalescer() override { return mCoalescer; }
		[[nodiscard]] const InputCoalescer& coalescer() const override { return mCoalescer; }
		[[nodiscard]] bool isWaitingForFrame() const override;
		void flushPendingInput() override;

	private:
		friend class WaylandSeat;
		friend struct WaylandListeners;

		void applyConfiguration(const WindowConfigure& configuration);
		void applyScale(halnf scale);
		void notifyOwnerResized();
		[[nodiscard]] bool pollOnce(int timeoutMs);
		void updateFrameGeometry();
		void requestFrameCallback();

		void onPointerSurfaceMove(Vec2F surfacePoint);
		void onPointerSurfaceButton(uint32_t button, bool pressed, uint32_t serial);
		void onPointerSurfaceLeft();
		void onPointerSurfacePinch(const TouchGesture& gesture);
		[[nodiscard]] Vec2F toContent(Vec2F surfacePoint) const;
		[[nodiscard]] std::string activationToken();

		Window& mOwner;
		bool mAcceptsInput = true;
		std::unique_ptr<WaylandConnection> mConnection;
		std::unique_ptr<WaylandSeat> mSeat;
		int mWakeDescriptor = -1;

		struct Objects;
		std::unique_ptr<Objects> mObjects;

		WindowFrame mFrame;
		Vec2F mRestoredContentSize;
		halnf mScale = 1;
		bool mConfigured = false;
		InputCoalescer mCoalescer;
		bool mCloseRequested = false;
		bool mMaximized = false;
		bool mFullScreen = false;
		bool mActivated = false;
		bool mGeometryDirty = true;
		WindowEdge mHoveredEdge = WindowEdge::None;
		int mHeldButtons = 0;
		CursorShape mClientCursor = CursorShape::Default;
	};
}
