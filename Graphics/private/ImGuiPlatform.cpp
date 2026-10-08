#include "ImGuiPlatform.hpp"

#include "Timing.hpp"
#include "Window.hpp"

#include <imgui.h>

#include <cfloat>
#include <unordered_map>

using namespace tp;

namespace {
	constexpr double kFirstFrameSeconds = 1.0 / 60.0;

	ImGuiKey imguiKey(InputID key) {
		static const std::unordered_map<InputID, ImGuiKey> keys = {
			{ InputID::TAB, ImGuiKey_Tab },
			{ InputID::LEFT, ImGuiKey_LeftArrow },
			{ InputID::RIGHT, ImGuiKey_RightArrow },
			{ InputID::UP, ImGuiKey_UpArrow },
			{ InputID::DOWN, ImGuiKey_DownArrow },
			{ InputID::PAGE_UP, ImGuiKey_PageUp },
			{ InputID::PAGE_DOWN, ImGuiKey_PageDown },
			{ InputID::HOME, ImGuiKey_Home },
			{ InputID::END, ImGuiKey_End },
			{ InputID::INSERT, ImGuiKey_Insert },
			{ InputID::DELETE_KEY, ImGuiKey_Delete },
			{ InputID::BACKSPACE, ImGuiKey_Backspace },
			{ InputID::SPACE, ImGuiKey_Space },
			{ InputID::ENTER, ImGuiKey_Enter },
			{ InputID::ESCAPE, ImGuiKey_Escape },
			{ InputID::LEFT_CONTROL, ImGuiKey_LeftCtrl },
			{ InputID::LEFT_SHIFT, ImGuiKey_LeftShift },
			{ InputID::LEFT_ALT, ImGuiKey_LeftAlt },
			{ InputID::LEFT_SUPER, ImGuiKey_LeftSuper },
			{ InputID::RIGHT_CONTROL, ImGuiKey_RightCtrl },
			{ InputID::RIGHT_SHIFT, ImGuiKey_RightShift },
			{ InputID::RIGHT_ALT, ImGuiKey_RightAlt },
			{ InputID::RIGHT_SUPER, ImGuiKey_RightSuper },
			{ InputID::MENU, ImGuiKey_Menu },
			{ InputID::APOSTROPHE, ImGuiKey_Apostrophe },
			{ InputID::COMMA, ImGuiKey_Comma },
			{ InputID::MINUS, ImGuiKey_Minus },
			{ InputID::PERIOD, ImGuiKey_Period },
			{ InputID::SLASH, ImGuiKey_Slash },
			{ InputID::SEMICOLON, ImGuiKey_Semicolon },
			{ InputID::EQUAL, ImGuiKey_Equal },
			{ InputID::LEFT_BRACKET, ImGuiKey_LeftBracket },
			{ InputID::BACKSLASH, ImGuiKey_Backslash },
			{ InputID::RIGHT_BRACKET, ImGuiKey_RightBracket },
			{ InputID::GRAVE_ACCENT, ImGuiKey_GraveAccent },
			{ InputID::CAPS_LOCK, ImGuiKey_CapsLock },
			{ InputID::SCROLL_LOCK, ImGuiKey_ScrollLock },
			{ InputID::NUM_LOCK, ImGuiKey_NumLock },
			{ InputID::PRINT_SCREEN, ImGuiKey_PrintScreen },
			{ InputID::PAUSE, ImGuiKey_Pause },
			{ InputID::KP_DECIMAL, ImGuiKey_KeypadDecimal },
			{ InputID::KP_DIVIDE, ImGuiKey_KeypadDivide },
			{ InputID::KP_MULTIPLY, ImGuiKey_KeypadMultiply },
			{ InputID::KP_SUBTRACT, ImGuiKey_KeypadSubtract },
			{ InputID::KP_ADD, ImGuiKey_KeypadAdd },
			{ InputID::KP_ENTER, ImGuiKey_KeypadEnter },
			{ InputID::KP_EQUAL, ImGuiKey_KeypadEqual },
		};

		const auto found = keys.find(key);
		if (found != keys.end()) return found->second;

		const auto code = (int) key;
		if (code >= (int) InputID::A && code <= (int) InputID::Z) return (ImGuiKey) (ImGuiKey_A + code - (int) InputID::A);
		if (code >= (int) InputID::N0 && code <= (int) InputID::N9)
			return (ImGuiKey) (ImGuiKey_0 + code - (int) InputID::N0);
		if (code >= (int) InputID::F1 && code <= (int) InputID::F24)
			return (ImGuiKey) (ImGuiKey_F1 + code - (int) InputID::F1);
		if (code >= (int) InputID::KP_0 && code <= (int) InputID::KP_9)
			return (ImGuiKey) (ImGuiKey_Keypad0 + code - (int) InputID::KP_0);
		return ImGuiKey_None;
	}

	ImGuiKey imguiModifier(InputID key) {
		switch (key) {
			case InputID::LEFT_CONTROL:
			case InputID::RIGHT_CONTROL: return ImGuiMod_Ctrl;
			case InputID::LEFT_SHIFT:
			case InputID::RIGHT_SHIFT: return ImGuiMod_Shift;
			case InputID::LEFT_ALT:
			case InputID::RIGHT_ALT: return ImGuiMod_Alt;
			case InputID::LEFT_SUPER:
			case InputID::RIGHT_SUPER: return ImGuiMod_Super;
			default: return ImGuiKey_None;
		}
	}

	bool isRightModifier(InputID key) {
		return key == InputID::RIGHT_CONTROL || key == InputID::RIGHT_SHIFT || key == InputID::RIGHT_ALT ||
					 key == InputID::RIGHT_SUPER;
	}

	int modifierIndex(ImGuiKey modifier) {
		switch (modifier) {
			case ImGuiMod_Ctrl: return 0;
			case ImGuiMod_Shift: return 1;
			case ImGuiMod_Alt: return 2;
			default: return 3;
		}
	}

	int imguiMouseButton(InputID button) {
		switch (button) {
			case InputID::MOUSE1: return ImGuiMouseButton_Left;
			case InputID::MOUSE2: return ImGuiMouseButton_Right;
			case InputID::MOUSE3: return ImGuiMouseButton_Middle;
			case InputID::MOUSE4: return 3;
			case InputID::MOUSE5: return 4;
			default: return -1;
		}
	}

	CursorShape cursorShape(ImGuiMouseCursor cursor) {
		switch (cursor) {
			case ImGuiMouseCursor_TextInput: return CursorShape::Text;
			case ImGuiMouseCursor_ResizeAll: return CursorShape::Move;
			case ImGuiMouseCursor_ResizeNS: return CursorShape::ResizeVertical;
			case ImGuiMouseCursor_ResizeEW: return CursorShape::ResizeHorizontal;
			case ImGuiMouseCursor_ResizeNESW: return CursorShape::ResizeNorthEastSouthWest;
			case ImGuiMouseCursor_ResizeNWSE: return CursorShape::ResizeNorthWestSouthEast;
			case ImGuiMouseCursor_Hand: return CursorShape::Pointer;
			case ImGuiMouseCursor_NotAllowed: return CursorShape::NotAllowed;
			default: return CursorShape::Default;
		}
	}
}

ImGuiPlatform::ImGuiPlatform(Window& window) :
	mWindow(window) {
	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = "modules_wayland";
	io.BackendPlatformUserData = this;
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.SetClipboardTextFn = &ImGuiPlatform::writeClipboard;
	io.GetClipboardTextFn = &ImGuiPlatform::readClipboard;
	io.ClipboardUserData = this;
	io.SetPlatformImeDataFn = &ImGuiPlatform::placeInputMethod;

	mWindow.addInputObserver(this);
}

ImGuiPlatform::~ImGuiPlatform() {
	mWindow.removeInputObserver(this);

	ImGuiIO& io = ImGui::GetIO();
	io.BackendPlatformName = nullptr;
	io.BackendPlatformUserData = nullptr;
	io.SetClipboardTextFn = nullptr;
	io.GetClipboardTextFn = nullptr;
	io.SetPlatformImeDataFn = nullptr;
}

void ImGuiPlatform::newFrame() {
	ImGuiIO& io = ImGui::GetIO();
	const Vec2F size = mWindow.getSize();
	const Vec2F framebuffer = mWindow.getFramebufferSize();
	const Vec2F surface = mWindow.frame().surfaceSize();
	const halnf scale = surface.x > 0 ? framebuffer.x / surface.x : 1;

	io.DisplaySize = ImVec2(size.x, size.y);
	io.DisplayFramebufferScale = ImVec2(scale, scale);

	const double now = (double) get_time_ns() / 1.0e9;
	io.DeltaTime = mPreviousTime > 0 && now > mPreviousTime ? (float) (now - mPreviousTime) : (float) kFirstFrameSeconds;
	mPreviousTime = now;

	updateCursor();
}

void ImGuiPlatform::endFrame() {
	ImDrawData* drawData = ImGui::GetDrawData();
	if (!drawData) return;

	const halnf margin = mWindow.frame().margin();
	drawData->DisplayPos = ImVec2(-margin, -margin);
	drawData->DisplaySize = ImVec2(mWindow.frame().surfaceSize().x, mWindow.frame().surfaceSize().y);

	const bool wantsText = ImGui::GetIO().WantTextInput;
	if (wantsText != mTextInputWanted) {
		mTextInputWanted = wantsText;
		mWindow.setTextInputEnabled(wantsText);
	}
}

void ImGuiPlatform::onPointerMoved(Vec2F contentPosition) {
	ImGui::GetIO().AddMousePosEvent(contentPosition.x, contentPosition.y);
}

void ImGuiPlatform::onPointerLeft() { ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX); }

void ImGuiPlatform::onPointerButton(InputID button, bool pressed) {
	const int index = imguiMouseButton(button);
	if (index >= 0) ImGui::GetIO().AddMouseButtonEvent(index, pressed);
}

void ImGuiPlatform::onScrolled(Vec2F delta) { ImGui::GetIO().AddMouseWheelEvent(delta.x, delta.y); }

void ImGuiPlatform::onKey(InputID key, bool pressed) {
	updateModifier(key, pressed);

	const ImGuiKey imgui = imguiKey(key);
	if (imgui != ImGuiKey_None) ImGui::GetIO().AddKeyEvent(imgui, pressed);
}

void ImGuiPlatform::onKeyRepeat(InputID key) { onKey(key, true); }

void ImGuiPlatform::onText(const std::string& utf8) { ImGui::GetIO().AddInputCharactersUTF8(utf8.c_str()); }

void ImGuiPlatform::onKeyboardFocus(bool focused) { ImGui::GetIO().AddFocusEvent(focused); }

void ImGuiPlatform::updateModifier(InputID key, bool pressed) {
	const ImGuiKey modifier = imguiModifier(key);
	if (modifier == ImGuiKey_None) return;

	const size_t side = isRightModifier(key) ? 1 : 0;
	mModifierSides[(size_t) modifierIndex(modifier)][side] = pressed;

	const bool down =
		mModifierSides[(size_t) modifierIndex(modifier)][0] || mModifierSides[(size_t) modifierIndex(modifier)][1];
	ImGui::GetIO().AddKeyEvent(modifier, down);
}

void ImGuiPlatform::updateCursor() {
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) return;
	if (!io.WantCaptureMouse) {
		mCursor = -2;
		return;
	}

	const ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
	if (cursor == mCursor) return;

	mCursor = cursor;
	mWindow.setCursor(cursorShape(cursor));
}

const char* ImGuiPlatform::readClipboard(void* userData) {
	auto* self = static_cast<ImGuiPlatform*>(userData);
	self->mClipboard = self->mWindow.clipboardText();
	return self->mClipboard.c_str();
}

void ImGuiPlatform::writeClipboard(void* userData, const char* text) {
	static_cast<ImGuiPlatform*>(userData)->mWindow.setClipboardText(text ? text : "");
}

void ImGuiPlatform::placeInputMethod(ImGuiViewport*, ImGuiPlatformImeData* data) {
	auto* self = static_cast<ImGuiPlatform*>(ImGui::GetIO().BackendPlatformUserData);
	if (!self || !data->WantVisible) return;

	self->mWindow.setTextInputRect({ data->InputPos.x, data->InputPos.y, 1, data->InputLineHeight });
}
