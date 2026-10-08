#include "AndroidWindow.hpp"

#include "AndroidKeys.hpp"
#include "AndroidPlatform.hpp"
#include "Window.hpp"
#include "WindowContext.hpp"
#include "WindowInputDispatcher.hpp"

#include <android/configuration.h>
#include <sys/system_properties.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan_android.h>

#include <algorithm>
#include <optional>
#include <cmath>

namespace tp {

	namespace {
		constexpr const char* kTag = "modules";
		constexpr halnf kReferenceDensity = 160.0f;
		constexpr halnf kWheelStep = 1.0f;

		PointerTool toolFor(int32_t type) {
			switch (type) {
				case AMOTION_EVENT_TOOL_TYPE_STYLUS: return PointerTool::Pen;
				case AMOTION_EVENT_TOOL_TYPE_ERASER: return PointerTool::Eraser;
				case AMOTION_EVENT_TOOL_TYPE_FINGER: return PointerTool::Finger;
				default: return PointerTool::Mouse;
			}
		}

		std::optional<InputID> buttonFor(int32_t androidButton) {
			switch (androidButton) {
				case AMOTION_EVENT_BUTTON_PRIMARY: return InputID::MOUSE1;
				case AMOTION_EVENT_BUTTON_SECONDARY: return InputID::MOUSE2;
				case AMOTION_EVENT_BUTTON_TERTIARY: return InputID::MOUSE3;
				case AMOTION_EVENT_BUTTON_BACK: return InputID::MOUSE4;
				case AMOTION_EVENT_BUTTON_FORWARD: return InputID::MOUSE5;
				default: return std::nullopt;
			}
		}

		class MutedInput : public WindowInputObserver {};
	}

	AndroidWindow::AndroidWindow(Window& owner, android_app& app) :
		mOwner(owner),
		mApp(app) {
		mFrame.setTranslucent(false);
		mFrame.setTiled(true);

		mApp.userData = this;
		mApp.onAppCmd = onCommand;
		mApp.onInputEvent = onInput;

		if (mApp.config) {
			const int32_t density = AConfiguration_getDensity(mApp.config);
			if (density > 0 && density != ACONFIGURATION_DENSITY_ANY) mScale = (halnf) density / kReferenceDensity;
		}

		while (!mApp.window && !mApp.destroyRequested)
			pollOnce(-1);
		if (mApp.window) attachWindow(mApp.window);
	}

	AndroidWindow::~AndroidWindow() {
		mApp.onAppCmd = nullptr;
		mApp.onInputEvent = nullptr;
		mApp.userData = nullptr;
	}

	std::vector<const char*> AndroidWindow::requiredInstanceExtensions() const {
		return { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME };
	}

	VkSurfaceKHR AndroidWindow::createSurface(VkInstance instance) const {
		if (!mNativeWindow) return VK_NULL_HANDLE;
		VkAndroidSurfaceCreateInfoKHR info{
			.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			.window = mNativeWindow,
		};
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		if (vkCreateAndroidSurfaceKHR(instance, &info, nullptr, &surface) != VK_SUCCESS) return VK_NULL_HANDLE;
		return surface;
	}

	VkExtent2D AndroidWindow::framebufferExtent() const {
		if (!mNativeWindow) return { 0, 0 };
		return { (uint32_t) ANativeWindow_getWidth(mNativeWindow), (uint32_t) ANativeWindow_getHeight(mNativeWindow) };
	}

	void AndroidWindow::onTranslucencyResolved(bool) { mFrame.setTranslucent(false); }

	void AndroidWindow::updateGeometry() {
		if (!mNativeWindow) return;
		const halnf width = (halnf) ANativeWindow_getWidth(mNativeWindow) / mScale;
		const halnf height = (halnf) ANativeWindow_getHeight(mNativeWindow) / mScale;
		if (width <= 0 || height <= 0) return;

		const EdgeInsets pixels = android::systemInsets(mApp);
		const EdgeInsets insets = {
			pixels.left / mScale, pixels.top / mScale, pixels.right / mScale, pixels.bottom / mScale
		};
		const bool sizeChanged = Vec2F(width, height) != mFrame.contentSize();
		if (!sizeChanged && insets == mSafeInsets) return;

		mSafeInsets = insets;
		mFrame.setContentSize({ width, height });
		if (mConfigured) mOwner.notifyResized();
	}

	void AndroidWindow::attachWindow(ANativeWindow* window) {
		mNativeWindow = window;
		updateGeometry();
		if (VulkanContext* vulkan = mOwner.vulkan(); vulkan && !vulkan->hasSurface()) vulkan->recreateSurface();
		__android_log_print(
			ANDROID_LOG_INFO,
			kTag,
			"window %dx%d scale %.2f",
			ANativeWindow_getWidth(window),
			ANativeWindow_getHeight(window),
			mScale
		);
	}

	void AndroidWindow::detachWindow() {
		if (VulkanContext* vulkan = mOwner.vulkan()) vulkan->releaseSurface();
		mNativeWindow = nullptr;
	}

	void AndroidWindow::onCommand(android_app* app, int32_t command) {
		if (auto* self = static_cast<AndroidWindow*>(app->userData)) self->handleCommand(command);
	}

	int32_t AndroidWindow::onInput(android_app* app, AInputEvent* event) {
		auto* self = static_cast<AndroidWindow*>(app->userData);
		return self && self->handleInput(event) ? 1 : 0;
	}

	void AndroidWindow::handleCommand(int32_t command) {
		switch (command) {
			case APP_CMD_INIT_WINDOW:
				if (mApp.window) attachWindow(mApp.window);
				break;
			case APP_CMD_TERM_WINDOW: detachWindow(); break;
			case APP_CMD_WINDOW_RESIZED:
			case APP_CMD_CONFIG_CHANGED:
			case APP_CMD_CONTENT_RECT_CHANGED: updateGeometry(); break;
			case APP_CMD_GAINED_FOCUS:
				mFocused = true;
				platformInput().onKeyboardFocus(true);
				break;
			case APP_CMD_LOST_FOCUS:
				mFocused = false;
				platformInput().onKeyboardFocus(false);
				break;
			default: break;
		}
	}

	WindowInputObserver& AndroidWindow::platformInput() {
		static MutedInput muted;
		if (!mAcceptsInput) return muted;
		return mCoalescer.recorder();
	}

	bool AndroidWindow::handleInput(AInputEvent* event) {
		switch (AInputEvent_getType(event)) {
			case AINPUT_EVENT_TYPE_MOTION: return handleMotion(event);
			case AINPUT_EVENT_TYPE_KEY: return handleKey(event);
			default: return false;
		}
	}

	bool AndroidWindow::handleMotion(AInputEvent* event) {
		const int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
		const int32_t toolType = AMotionEvent_getToolType(event, 0);
		if (toolType == AMOTION_EVENT_TOOL_TYPE_STYLUS || toolType == AMOTION_EVENT_TOOL_TYPE_ERASER) mPenSeen = true;
		if (toolType == AMOTION_EVENT_TOOL_TYPE_MOUSE) mMouseSeen = true;
		WindowInputObserver& input = platformInput();

		if (toolType == AMOTION_EVENT_TOOL_TYPE_FINGER && handleTouchGesture(event, action)) return true;

		const auto position = [&](size_t history) -> Vec2F {
			if (history == SIZE_MAX) return { AMotionEvent_getX(event, 0) / mScale, AMotionEvent_getY(event, 0) / mScale };
			return { AMotionEvent_getHistoricalX(event, 0, history) / mScale,
							 AMotionEvent_getHistoricalY(event, 0, history) / mScale };
		};
		const auto sample = [&](size_t history) {
			PointerSample result;
			result.tool = toolFor(toolType);
			const bool current = history == SIZE_MAX;
			result.pressure =
				current ? AMotionEvent_getPressure(event, 0) : AMotionEvent_getHistoricalPressure(event, 0, history);
			const halnf tilt = current ? AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_TILT, 0)
																 : AMotionEvent_getHistoricalAxisValue(event, AMOTION_EVENT_AXIS_TILT, 0, history);
			const halnf orientation =
				current ? AMotionEvent_getOrientation(event, 0) : AMotionEvent_getHistoricalOrientation(event, 0, history);
			result.tilt = { std::sin(orientation) * tilt, -std::cos(orientation) * tilt };
			result.rotation = orientation;
			result.timeMs =
				(uint32_t) ((current ? AMotionEvent_getEventTime(event) : AMotionEvent_getHistoricalEventTime(event, history)) /
										1000000);
			if (!result.isStylus()) result.pressure = 1;
			else if (result.pressure > 0) mLastContactPressure = result.pressure;
			return result;
		};

		switch (action) {
			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_BUTTON_PRESS:
				{
					input.onPointerSample(sample(SIZE_MAX));
					input.onPointerMoved(position(SIZE_MAX));
					if (action == AMOTION_EVENT_ACTION_BUTTON_PRESS) {
						deliverButtonChanges(input, AMotionEvent_getButtonState(event));
					} else if (
						!mPrimaryHeld && (AMotionEvent_getButtonState(event) == 0 || toolType != AMOTION_EVENT_TOOL_TYPE_MOUSE)
					) {
						mPrimaryHeld = true;
						input.onPointerButton(InputID::MOUSE1, true);
					}
					return true;
				}
			case AMOTION_EVENT_ACTION_MOVE:
			case AMOTION_EVENT_ACTION_HOVER_MOVE:
				{
					const size_t history = AMotionEvent_getHistorySize(event);
					for (size_t index = 0; index < history; index++) {
						input.onPointerSample(sample(index));
						input.onPointerMoved(position(index));
					}
					input.onPointerSample(sample(SIZE_MAX));
					input.onPointerMoved(position(SIZE_MAX));
					return true;
				}
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_CANCEL:
			case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
				{
					PointerSample released = sample(SIZE_MAX);
					if (released.isStylus()) released.pressure = mLastContactPressure;
					input.onPointerSample(released);
					input.onPointerMoved(position(SIZE_MAX));
					if (action == AMOTION_EVENT_ACTION_BUTTON_RELEASE) {
						deliverButtonChanges(input, AMotionEvent_getButtonState(event));
					} else if (mPrimaryHeld) {
						mPrimaryHeld = false;
						input.onPointerButton(InputID::MOUSE1, false);
					}
					if (toolType == AMOTION_EVENT_TOOL_TYPE_FINGER) input.onPointerLeft();
					return true;
				}
			case AMOTION_EVENT_ACTION_HOVER_EXIT: input.onPointerLeft(); return true;
			case AMOTION_EVENT_ACTION_SCROLL:
				{
					const Vec2F delta = { -AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HSCROLL, 0) * kWheelStep,
																AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_VSCROLL, 0) * kWheelStep };
					input.onPointerMoved(position(SIZE_MAX));
					input.onScrolled(delta);
					return true;
				}
			default: return false;
		}
	}

	bool AndroidWindow::handleTouchGesture(AInputEvent* event, int32_t action) {
		const size_t count = AMotionEvent_getPointerCount(event);
		const bool lifting = action == AMOTION_EVENT_ACTION_POINTER_UP;
		const size_t liftedIndex =
			lifting ? (size_t) ((AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
													AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT)
							: SIZE_MAX;
		const bool ending = action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL;

		std::vector<TouchPoint> touches;
		if (!ending) {
			for (size_t index = 0; index < count; index++) {
				if (index == liftedIndex) continue;
				touches.push_back(
					{ AMotionEvent_getPointerId(event, index),
						{ AMotionEvent_getX(event, index) / mScale, AMotionEvent_getY(event, index) / mScale } }
				);
			}
		}

		if (touches.size() < 2 && !mGestures.isActive() && !mGestureHoldsPointer) return false;

		WindowInputObserver& input = platformInput();
		std::vector<TouchGesture> gestures;
		mGestures.update(touches, gestures, (uint32_t) (AMotionEvent_getEventTime(event) / 1000000));
		for (const TouchGesture& gesture : gestures)
			input.onGesture(gesture);

		if (touches.size() >= 2 && !mGestureHoldsPointer) {
			mGestureHoldsPointer = true;
			if (mPrimaryHeld) {
				mPrimaryHeld = false;
				input.onPointerButton(InputID::MOUSE1, false);
			}
			input.onPointerLeft();
		}

		if (ending) mGestureHoldsPointer = false;
		return true;
	}

	void AndroidWindow::deliverButtonChanges(WindowInputObserver& input, int32_t state) {
		const int32_t changed = state ^ mButtonState;
		for (int32_t bit = 1; bit <= AMOTION_EVENT_BUTTON_FORWARD; bit <<= 1) {
			if (!(changed & bit)) continue;
			if (auto button = buttonFor(bit)) input.onPointerButton(*button, (state & bit) != 0);
		}
		mButtonState = state;
	}

	bool AndroidWindow::handleKey(AInputEvent* event) {
		const int32_t keyCode = AKeyEvent_getKeyCode(event);
		if (keyCode == AKEYCODE_VOLUME_UP || keyCode == AKEYCODE_VOLUME_DOWN) return false;

		const int32_t action = AKeyEvent_getAction(event);
		WindowInputObserver& input = platformInput();
		const auto id = keyCode == AKEYCODE_BACK ? std::optional(InputID::ESCAPE) : android::inputFromKeyCode(keyCode);
		if (id) {
			if (action == AKEY_EVENT_ACTION_DOWN) {
				if (AKeyEvent_getRepeatCount(event) > 0) input.onKeyRepeat(*id);
				else input.onKey(*id, true);
			} else if (action == AKEY_EVENT_ACTION_UP) {
				input.onKey(*id, false);
			}
		}

		if (action == AKEY_EVENT_ACTION_DOWN) {
			const std::string text = android::unicodeText(mApp, event);
			if (!text.empty()) input.onText(text);
		}
		return true;
	}

	void AndroidWindow::pollOnce(int timeoutMs) {
		int events = 0;
		android_poll_source* source = nullptr;
		if (ALooper_pollOnce(timeoutMs, nullptr, &events, (void**) &source) >= 0 && source) source->process(&mApp, source);
	}

	void AndroidWindow::wake() {
		if (mApp.looper) ALooper_wake(mApp.looper);
	}

	void AndroidWindow::dispatchEvents(halnf timeoutMs) {
		int timeout = timeoutMs < 0 ? -1 : (int) std::ceil(timeoutMs);
		if (!mNativeWindow && timeout >= 0) timeout = 100;
		pollOnce(timeout);
		while (true) {
			int events = 0;
			android_poll_source* source = nullptr;
			if (ALooper_pollOnce(0, nullptr, &events, (void**) &source) < 0) break;
			if (source) source->process(&mApp, source);
			if (mApp.destroyRequested) break;
		}
	}

	void AndroidWindow::waitUntilConfigured() {
		while (!mNativeWindow && !mApp.destroyRequested)
			pollOnce(-1);
		updateGeometry();
		mConfigured = true;
		mOwner.notifyResized();
		__android_log_print(ANDROID_LOG_INFO, kTag, "platform %s", describe(platformInfo()).c_str());
	}

	void AndroidWindow::flushPendingInput() { mCoalescer.deliver(mOwner.dispatcher()); }

	bool AndroidWindow::shouldClose() const { return mApp.destroyRequested != 0; }

	void AndroidWindow::requestClose() { ANativeActivity_finish(mApp.activity); }

	void AndroidWindow::minimize() { ANativeActivity_finish(mApp.activity); }

	bool AndroidWindow::openUrl(const std::string& url) { return android::openUrl(mApp, url); }

	void AndroidWindow::setTextInputEnabled(bool enabled) {
		if (enabled == mTextInput) return;
		mTextInput = enabled;
		android::setSoftKeyboardVisible(mApp, enabled);
	}

	namespace {
		std::string systemProperty(const char* name) {
			char value[PROP_VALUE_MAX] = {};
			__system_property_get(name, value);
			return value;
		}

		constexpr int32_t kTabletSmallestWidthDp = 600;
	}

	PlatformInfo AndroidWindow::platformInfo() const {
		PlatformInfo info;
		info.system = OperatingSystem::Android;
		info.systemVersion = systemProperty("ro.build.version.release");
		info.deviceName = systemProperty("ro.product.manufacturer") + " " + systemProperty("ro.product.model");
		info.contentScale = mScale;
		info.screenSize = mFrame.contentSize();
		info.formFactor = FormFactor::Phone;
		if (AConfiguration* config = mApp.config) {
			const int32_t smallest = AConfiguration_getSmallestScreenWidthDp(config);
			if (smallest >= kTabletSmallestWidthDp) info.formFactor = FormFactor::Tablet;
			if (AConfiguration_getUiModeType(config) == ACONFIGURATION_UI_MODE_TYPE_DESK)
				info.formFactor = FormFactor::Desktop;
			info.hasTouch = AConfiguration_getTouchscreen(config) == ACONFIGURATION_TOUCHSCREEN_FINGER;
			info.hasPhysicalKeyboard = AConfiguration_getKeyboard(config) == ACONFIGURATION_KEYBOARD_QWERTY;
		}
		info.safeInsets = mSafeInsets;
		info.hasPen = mPenSeen;
		info.hasMouse = mMouseSeen;
		info.hasHoverPointer = mMouseSeen || mPenSeen;
		info.hasWindowControls = false;
		info.canMoveWindow = false;
		info.canResizeWindow = false;
		return info;
	}

	std::unique_ptr<PlatformWindow> createPlatformWindow(Window& owner, Vec2F, const WindowIdentity&) {
		android_app* app = android::application();
		if (!app) throw std::runtime_error("android_app is not set");
		return std::make_unique<AndroidWindow>(owner, *app);
	}
}
