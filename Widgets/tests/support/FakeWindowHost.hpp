#pragma once

#include "WindowHost.hpp"

#include <string>
#include <vector>

namespace tp::ui {

	class FakeWindowHost : public WindowHost {
	public:
		FakeWindowHost() {
			platform.formFactor = FormFactor::Desktop;
			platform.hasWindowControls = true;
			platform.canMoveWindow = true;
			platform.canResizeWindow = true;
		}

		[[nodiscard]] PlatformInfo platformInfo() const override { return platform; }
		void minimizeWindow() override { minimizeCount++; }
		void toggleMaximizeWindow() override {
			toggleMaximizeCount++;
			maximized = !maximized;
		}
		void closeWindow() override { closeCount++; }
		[[nodiscard]] bool isWindowMaximized() const override { return maximized; }
		void beginWindowMove() override { moveCount++; }
		void showWindowMenu(Vec2F windowPoint) override {
			windowMenuCount++;
			windowMenuPoint = windowPoint;
		}
		void setCursor(CursorShape shape) override { cursors.push_back(shape); }
		void setClipboardText(const std::string& text) override { clipboard = text; }
		[[nodiscard]] std::string clipboardText() override { return clipboard; }
		[[nodiscard]] ClipboardImage clipboardImage() override { return image; }
		bool openUrl(const std::string& url) override {
			openedUrls.push_back(url);
			return opensUrls;
		}
		[[nodiscard]] AppStorage persistentAppStorage() const override { return storage; }
		void setTextInputArea(const RectF* windowRect) override {
			textInputEnabled = windowRect != nullptr;
			if (windowRect) textInputRect = *windowRect;
		}

		PlatformInfo platform;
		int minimizeCount = 0;
		int toggleMaximizeCount = 0;
		int closeCount = 0;
		int moveCount = 0;
		int windowMenuCount = 0;
		Vec2F windowMenuPoint = { 0, 0 };
		bool maximized = false;
		std::vector<CursorShape> cursors;
		std::string clipboard;
		ClipboardImage image;
		std::vector<std::string> openedUrls;
		bool opensUrls = true;
		bool textInputEnabled = false;
		RectF textInputRect = makeRect(0, 0, 0, 0);
		AppStorage storage{ "/tmp/fake-app" };
	};
}
