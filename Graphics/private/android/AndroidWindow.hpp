#pragma once

#include "PlatformWindow.hpp"
#include "TouchGesture.hpp"
#include "WindowInput.hpp"

struct android_app;
struct ANativeWindow;
struct AInputEvent;

namespace tp {

	class AndroidWindow final : public PlatformWindow {
	public:
		AndroidWindow(Window& owner, android_app& app);
		~AndroidWindow() override;

		AndroidWindow(const AndroidWindow&) = delete;
		AndroidWindow& operator=(const AndroidWindow&) = delete;

		[[nodiscard]] std::vector<const char*> requiredInstanceExtensions() const override;
		[[nodiscard]] VkSurfaceKHR createSurface(VkInstance instance) const override;
		[[nodiscard]] VkExtent2D framebufferExtent() const override;
		[[nodiscard]] bool wantsTranslucency() const override { return false; }
		void onTranslucencyResolved(bool translucent) override;

		void setAcceptsInput(bool accepts) override { mAcceptsInput = accepts; }

		void dispatchEvents(halnf timeoutMs) override;
		void wake() override;
		void waitUntilConfigured() override;
		void commitFrameGeometry() override {}
		void flushPendingInput() override;
		[[nodiscard]] bool isWaitingForFrame() const override { return mNativeWindow == nullptr; }

		[[nodiscard]] bool shouldClose() const override;
		void requestClose() override;

		[[nodiscard]] const WindowFrame& frame() const override { return mFrame; }
		[[nodiscard]] Vec2F contentSize() const override { return mFrame.contentSize(); }
		[[nodiscard]] halnf scale() const override { return mScale; }
		[[nodiscard]] halnf refreshIntervalMs() const override { return 1000.0f / 60.0f; }
		[[nodiscard]] bool isMaximized() const override { return true; }
		[[nodiscard]] bool isFocused() const override { return mFocused; }

		void minimize() override;
		void toggleMaximize() override {}
		void setFullScreen(bool) override {}
		[[nodiscard]] bool isFullScreen() const override { return true; }
		void beginMove() override {}
		void beginResize(WindowEdge) override {}
		void showWindowMenu(Vec2F) override {}

		void setCursor(CursorShape) override {}
		void setClipboardText(const std::string& text) override { mClipboard = text; }
		[[nodiscard]] std::string clipboardText() override { return mClipboard; }
		[[nodiscard]] ClipboardImage clipboardImage() override { return {}; }
		bool openUrl(const std::string& url) override;
		void setTextInputEnabled(bool enabled) override;
		void setTextInputRect(const RectF&) override {}

		[[nodiscard]] PlatformInfo platformInfo() const override;

		[[nodiscard]] InputCoalescer& coalescer() override { return mCoalescer; }
		[[nodiscard]] const InputCoalescer& coalescer() const override { return mCoalescer; }

		[[nodiscard]] bool hasSurface() const { return mNativeWindow != nullptr; }

	private:
		static void onCommand(android_app* app, int32_t command);
		static int32_t onInput(android_app* app, AInputEvent* event);

		void handleCommand(int32_t command);
		[[nodiscard]] bool handleInput(AInputEvent* event);
		[[nodiscard]] bool handleMotion(AInputEvent* event);
		[[nodiscard]] bool handleKey(AInputEvent* event);
		void deliverButtonChanges(WindowInputObserver& input, int32_t state);
		[[nodiscard]] bool handleTouchGesture(AInputEvent* event, int32_t action);

		void attachWindow(ANativeWindow* window);
		void detachWindow();
		void updateGeometry();
		void pollOnce(int timeoutMs);
		[[nodiscard]] WindowInputObserver& platformInput();

		Window& mOwner;
		android_app& mApp;
		ANativeWindow* mNativeWindow = nullptr;
		WindowFrame mFrame;
		InputCoalescer mCoalescer;
		std::string mClipboard;
		halnf mScale = 1;
		bool mAcceptsInput = true;
		bool mFocused = false;
		bool mConfigured = false;
		bool mPrimaryHeld = false;
		bool mTextInput = false;
		EdgeInsets mSafeInsets;
		bool mPenSeen = false;
		bool mMouseSeen = false;
		int32_t mButtonState = 0;
		halnf mLastContactPressure = 1;
		TouchGestureTracker mGestures;
		bool mGestureHoldsPointer = false;
	};
}
