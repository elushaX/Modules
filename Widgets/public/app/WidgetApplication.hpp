#pragma once

#include "ApplicationFrame.hpp"
#include "DebugInspector.hpp"
#include "DebugTools.hpp"
#include "GraphicApplication.hpp"
#include "InputTranslator.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

#include <memory>

namespace tp::ui {

	class CanvasPainter;
	class CanvasImageLoader;

	class WidgetApplication : public Application, private WindowHost, private SessionRecorderHost {
	public:
		explicit WidgetApplication(const WindowIdentity& identity = WindowIdentity::generic());
		~WidgetApplication() override;

		void setRoot(Widget& root);
		void clearRoot();

		[[nodiscard]] MenuBar& menuBar() { return mFrame.menuBar; }
		void setMenuBarVisible(bool visible) { mFrame.setMenuBarVisible(visible); }
		[[nodiscard]] bool isMenuBarVisible() const { return mFrame.isMenuBarVisible(); }
		[[nodiscard]] ApplicationFrame& frame() { return mFrame; }

		[[nodiscard]] Ui& ui() { return mUi; }
		[[nodiscard]] DebugInspector& inspector() { return mInspector; }
		[[nodiscard]] DebugTools& debugTools() { return mDebugTools; }
		void setDebugToolsEnabled(bool enabled);
		[[nodiscard]] Canvas& canvas() { return *mGraphics->getCanvas(); }
		[[nodiscard]] AppStorage persistentAppStorage() const override = 0;

	protected:
		virtual void drawOverlayInterface() {}
		void processFrame(EventHandler* eventHandler, halnf deltaTime) override;
		[[nodiscard]] SessionEnvironment environment() const override;
		[[nodiscard]] HangSettings hangSettings() const override;

	private:
		void drawFrame(Canvas* canvas) override;
		bool forceNewFrame() override;

		[[nodiscard]] PlatformInfo platformInfo() const override;
		void minimizeWindow() override;
		void toggleMaximizeWindow() override;
		void closeWindow() override;
		[[nodiscard]] bool isWindowMaximized() const override;
		void beginWindowMove() override;
		void showWindowMenu(Vec2F windowPoint) override;
		void setCursor(CursorShape shape) override;
		void setClipboardText(const std::string& text) override;
		[[nodiscard]] std::string clipboardText() override;
		[[nodiscard]] ClipboardImage clipboardImage() override;
		bool openUrl(const std::string& url) override;
		void setTextInputArea(const RectF* windowRect) override;

		void setRawInputTap(WindowInputObserver* tap) override;
		[[nodiscard]] double nowMs() const override;

		std::unique_ptr<CanvasPainter> mTextMeasurer;
		std::unique_ptr<CanvasImageLoader> mImageLoader;
		Ui mUi;
		ApplicationFrame mFrame;
		DebugInspector mInspector{ mUi };
		DebugTools mDebugTools{ *this, mInspector, mFrame };
		InputTranslator mInputTranslator;
		halnf mLastProcessMs = 0;
	};
}
