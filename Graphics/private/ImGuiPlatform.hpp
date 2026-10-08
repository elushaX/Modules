#pragma once

#include "WindowInput.hpp"

#include <string>

struct ImGuiViewport;
struct ImGuiPlatformImeData;

namespace tp {

	class Window;

	class ImGuiPlatform : public WindowInputObserver {
	public:
		explicit ImGuiPlatform(Window& window);
		~ImGuiPlatform() override;

		ImGuiPlatform(const ImGuiPlatform&) = delete;
		ImGuiPlatform& operator=(const ImGuiPlatform&) = delete;

		void newFrame();
		void endFrame();

		void onPointerMoved(Vec2F contentPosition) override;
		void onPointerLeft() override;
		void onPointerButton(InputID button, bool pressed) override;
		void onScrolled(Vec2F delta) override;
		void onKey(InputID key, bool pressed) override;
		void onKeyRepeat(InputID key) override;
		void onText(const std::string& utf8) override;
		void onKeyboardFocus(bool focused) override;

	private:
		static const char* readClipboard(void* userData);
		static void writeClipboard(void* userData, const char* text);
		static void placeInputMethod(ImGuiViewport* viewport, ImGuiPlatformImeData* data);

		void updateCursor();
		void updateModifier(InputID key, bool pressed);

		Window& mWindow;
		std::string mClipboard;
		double mPreviousTime = 0;
		int mCursor = -2;
		bool mModifierSides[4][2] = {};
		bool mTextInputWanted = false;
	};
}
