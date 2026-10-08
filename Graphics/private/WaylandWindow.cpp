#include "WaylandWindow.hpp"

#include <sys/eventfd.h>

#include "KeyMapping.hpp"
#include "WaylandConnection.hpp"
#include "WaylandSeat.hpp"
#include "Window.hpp"
#include "WindowInputDispatcher.hpp"

#include "Timing.hpp"

#include "fractional-scale-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>

#include <linux/input-event-codes.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <vector>

namespace tp {

	namespace {
		constexpr halnf kFractionalScaleDenominator = 120;
		constexpr halnf kMinimumContentWidth = 320;
		constexpr halnf kMinimumContentHeight = 200;
		constexpr int kConfigureWaitMs = 1000;
	}

	struct WaylandWindow::Objects {
		wl_surface* surface = nullptr;
		xdg_surface* shellSurface = nullptr;
		xdg_toplevel* toplevel = nullptr;
		wp_viewport* viewport = nullptr;
		wp_fractional_scale_v1* fractionalScale = nullptr;
		zxdg_toplevel_decoration_v1* decoration = nullptr;
		WindowConfigure pending;
		wl_callback* frameCallback = nullptr;
	};

	struct WaylandListeners {
		static WaylandWindow& window(void* data) { return *static_cast<WaylandWindow*>(data); }

		static void surfaceConfigure(void* data, xdg_surface*, uint32_t serial) {
			WaylandWindow& self = window(data);
			WindowConfigure configure = self.mObjects->pending;
			configure.serial = serial;
			self.mCoalescer.configure(configure);
		}

		static void toplevelConfigure(void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array* states) {
			WindowConfigure configuration;
			configuration.width = width;
			configuration.height = height;

			const auto* entries = static_cast<const uint32_t*>(states->data);
			const size_t count = states->size / sizeof(uint32_t);
			for (size_t index = 0; index < count; index++) {
				switch (entries[index]) {
					case XDG_TOPLEVEL_STATE_MAXIMIZED: configuration.maximized = true; break;
					case XDG_TOPLEVEL_STATE_FULLSCREEN: configuration.fullscreen = true; break;
					case XDG_TOPLEVEL_STATE_ACTIVATED: configuration.activated = true; break;
					case XDG_TOPLEVEL_STATE_TILED_LEFT:
					case XDG_TOPLEVEL_STATE_TILED_RIGHT:
					case XDG_TOPLEVEL_STATE_TILED_TOP:
					case XDG_TOPLEVEL_STATE_TILED_BOTTOM: configuration.tiled = true; break;
					default: break;
				}
			}

			window(data).mObjects->pending = configuration;
		}

		static void toplevelClose(void* data, xdg_toplevel*) { window(data).mCloseRequested = true; }
		static void toplevelBounds(void*, xdg_toplevel*, int32_t, int32_t) {}
		static void toplevelCapabilities(void*, xdg_toplevel*, wl_array*) {}

		static void preferredScale(void* data, wp_fractional_scale_v1*, uint32_t scale) {
			window(data).applyScale((halnf) scale / kFractionalScaleDenominator);
		}

		static void decorationConfigure(void*, zxdg_toplevel_decoration_v1*, uint32_t) {}

		static void frameDone(void* data, wl_callback* callback, uint32_t) {
			WaylandWindow& self = window(data);
			wl_callback_destroy(callback);
			if (self.mObjects->frameCallback == callback) self.mObjects->frameCallback = nullptr;
		}
	};

	namespace {
		const xdg_surface_listener kShellSurfaceListener = { WaylandListeners::surfaceConfigure };

		const xdg_toplevel_listener kToplevelListener = {
			WaylandListeners::toplevelConfigure,
			WaylandListeners::toplevelClose,
			WaylandListeners::toplevelBounds,
			WaylandListeners::toplevelCapabilities,
		};

		const wp_fractional_scale_v1_listener kFractionalScaleListener = { WaylandListeners::preferredScale };

		const zxdg_toplevel_decoration_v1_listener kDecorationListener = { WaylandListeners::decorationConfigure };
		const wl_callback_listener kFrameListener = { WaylandListeners::frameDone };

		uint32_t resizeEdgeCode(WindowEdge edge) {
			switch (edge) {
				case WindowEdge::Top: return XDG_TOPLEVEL_RESIZE_EDGE_TOP;
				case WindowEdge::Bottom: return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
				case WindowEdge::Left: return XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
				case WindowEdge::Right: return XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
				case WindowEdge::TopLeft: return XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
				case WindowEdge::TopRight: return XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
				case WindowEdge::BottomLeft: return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
				case WindowEdge::BottomRight: return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
				case WindowEdge::None: return XDG_TOPLEVEL_RESIZE_EDGE_NONE;
			}
			return XDG_TOPLEVEL_RESIZE_EDGE_NONE;
		}

		uint32_t scaledLength(halnf length, halnf scale) { return (uint32_t) std::lround(length * scale); }
	}

	WaylandWindow::WaylandWindow(Window& owner, Vec2F contentSize, const WindowIdentity& identity) :
		mOwner(owner),
		mConnection(std::make_unique<WaylandConnection>()),
		mObjects(std::make_unique<Objects>()),
		mWakeDescriptor(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
		mFrame.setContentSize(contentSize);
		mRestoredContentSize = contentSize;

		WaylandConnection& connection = *mConnection;
		Objects& objects = *mObjects;

		objects.surface = wl_compositor_create_surface(connection.compositor());
		objects.shellSurface = xdg_wm_base_get_xdg_surface(connection.windowManager(), objects.surface);
		xdg_surface_add_listener(objects.shellSurface, &kShellSurfaceListener, this);

		objects.toplevel = xdg_surface_get_toplevel(objects.shellSurface);
		xdg_toplevel_add_listener(objects.toplevel, &kToplevelListener, this);
		xdg_toplevel_set_title(objects.toplevel, identity.title.c_str());
		xdg_toplevel_set_app_id(objects.toplevel, identity.appId.c_str());
		xdg_toplevel_set_min_size(objects.toplevel, (int32_t) kMinimumContentWidth, (int32_t) kMinimumContentHeight);

		if (connection.decorationManager()) {
			objects.decoration =
				zxdg_decoration_manager_v1_get_toplevel_decoration(connection.decorationManager(), objects.toplevel);
			zxdg_toplevel_decoration_v1_add_listener(objects.decoration, &kDecorationListener, this);
			zxdg_toplevel_decoration_v1_set_mode(objects.decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
		}

		if (connection.viewporter())
			objects.viewport = wp_viewporter_get_viewport(connection.viewporter(), objects.surface);
		if (connection.fractionalScaleManager() && objects.viewport) {
			objects.fractionalScale =
				wp_fractional_scale_manager_v1_get_fractional_scale(connection.fractionalScaleManager(), objects.surface);
			wp_fractional_scale_v1_add_listener(objects.fractionalScale, &kFractionalScaleListener, this);
		}

		mSeat = std::make_unique<WaylandSeat>(connection, *this);

		updateFrameGeometry();
		wl_surface_commit(objects.surface);
		connection.flush();
	}

	WaylandWindow::~WaylandWindow() {
		if (mWakeDescriptor >= 0) close(mWakeDescriptor);
		mSeat.reset();

		Objects& objects = *mObjects;
		if (objects.frameCallback) wl_callback_destroy(objects.frameCallback);
		if (objects.decoration) zxdg_toplevel_decoration_v1_destroy(objects.decoration);
		if (objects.fractionalScale) wp_fractional_scale_v1_destroy(objects.fractionalScale);
		if (objects.viewport) wp_viewport_destroy(objects.viewport);
		if (objects.toplevel) xdg_toplevel_destroy(objects.toplevel);
		if (objects.shellSurface) xdg_surface_destroy(objects.shellSurface);
		if (objects.surface) wl_surface_destroy(objects.surface);
		mConnection->flush();
	}

	std::vector<const char*> WaylandWindow::requiredInstanceExtensions() const {
		return { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME };
	}

	VkSurfaceKHR WaylandWindow::createSurface(VkInstance instance) const {
		VkWaylandSurfaceCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
			.display = mConnection->display(),
			.surface = mObjects->surface,
		};

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		if (vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) return VK_NULL_HANDLE;
		return surface;
	}

	VkExtent2D WaylandWindow::framebufferExtent() const {
		const Vec2F surface = mFrame.surfaceSize();
		return { scaledLength(surface.x, mScale), scaledLength(surface.y, mScale) };
	}

	void WaylandWindow::onTranslucencyResolved(bool translucent) {
		mFrame.setTranslucent(translucent);
		mGeometryDirty = true;
	}

	halnf WaylandWindow::refreshIntervalMs() const { return 1.0e6f / (halnf) mConnection->refreshMillihertz(); }

	void WaylandWindow::dispatchEvents(halnf timeoutMs) {
		const bool waitsForever = timeoutMs < 0;
		const time_ns deadline = get_time_ns() + (time_ns) (std::max(timeoutMs, 0.0f) * 1.0e6f);

		while (true) {
			const halnf remainingMs = waitsForever ? -1.0f : (halnf) ((double) (deadline - get_time_ns()) / 1.0e6);
			if (pollOnce(waitsForever ? -1 : std::max(0, (int) std::ceil(remainingMs)))) return;
			if (!waitsForever && get_time_ns() >= deadline) return;
		}
	}

	void WaylandWindow::wake() {
		if (mWakeDescriptor >= 0) eventfd_write(mWakeDescriptor, 1);
	}

	bool WaylandWindow::pollOnce(int timeoutMs) {
		wl_display* display = mConnection->display();

		mConnection->flush();
		if (wl_display_dispatch_pending(display) > 0) return true;

		while (wl_display_prepare_read(display) != 0) {
			if (wl_display_dispatch_pending(display) > 0) return true;
		}
		mConnection->flush();

		const int repeatTimer = mSeat->repeatTimerDescriptor();
		pollfd descriptors[3] = {
			{ wl_display_get_fd(display), POLLIN, 0 },
			{ mWakeDescriptor, POLLIN, 0 },
			{ repeatTimer, POLLIN, 0 },
		};
		const nfds_t descriptorCount = repeatTimer >= 0 ? 3 : 2;

		const int ready = poll(descriptors, descriptorCount, timeoutMs);
		if (ready > 0 && (descriptors[0].revents & POLLIN)) {
			if (wl_display_read_events(display) < 0) throw std::runtime_error("lost the Wayland connection");
		} else {
			wl_display_cancel_read(display);
		}

		bool handled = wl_display_dispatch_pending(display) > 0;
		if (ready > 0 && (descriptors[1].revents & POLLIN)) {
			eventfd_t ignored = 0;
			eventfd_read(mWakeDescriptor, &ignored);
			handled = true;
		}
		if (ready > 0 && descriptorCount == 3 && (descriptors[2].revents & POLLIN)) {
			mSeat->onRepeatTimer();
			handled = true;
		}

		mConnection->flush();
		return handled;
	}

	void WaylandWindow::waitUntilConfigured() {
		while (!mConfigured) {
			if (!mConnection->dispatch(kConfigureWaitMs)) break;
			flushPendingInput();
		}
	}

	void WaylandWindow::applyConfiguration(const WindowConfigure& configuration) {
		mMaximized = configuration.maximized || configuration.fullscreen;
		mFullScreen = configuration.fullscreen;
		mActivated = configuration.activated;

		const bool tiled = mMaximized || configuration.tiled;
		if (tiled != mFrame.isTiled()) {
			mFrame.setTiled(tiled);
			mGeometryDirty = true;
		}

		Vec2F contentSize = mFrame.contentSize();
		if (configuration.width > 0 && configuration.height > 0) {
			contentSize = { (halnf) configuration.width, (halnf) configuration.height };
		} else if (!tiled) {
			contentSize = mRestoredContentSize;
		}

		contentSize.x = std::max(contentSize.x, kMinimumContentWidth);
		contentSize.y = std::max(contentSize.y, kMinimumContentHeight);
		if (!tiled) mRestoredContentSize = contentSize;

		if (contentSize != mFrame.contentSize()) {
			mFrame.setContentSize(contentSize);
			mGeometryDirty = true;
		}
	}

	void WaylandWindow::applyScale(halnf scale) {
		if (scale <= 0 || scale == mScale) return;

		mScale = scale;
		mGeometryDirty = true;
		if (mConfigured) notifyOwnerResized();
	}

	void WaylandWindow::notifyOwnerResized() { mOwner.notifyResized(); }

	namespace {
		class MutedInput : public WindowInputObserver {};
	}

	WindowInputObserver& WaylandWindow::platformInput() {
		static MutedInput muted;
		if (!mAcceptsInput) return muted;
		return mCoalescer.recorder();
	}

	void WaylandWindow::flushPendingInput() {
		if (mCoalescer.hasPendingConfigure()) {
			const WindowConfigure configure = mCoalescer.takeConfigure();
			applyConfiguration(configure);
			xdg_surface_ack_configure(mObjects->shellSurface, configure.serial);
			mConfigured = true;
			notifyOwnerResized();
		}

		mCoalescer.deliver(mOwner.dispatcher());
	}

	void WaylandWindow::setAcceptsInput(bool accepts) {
		mAcceptsInput = accepts;
		mGeometryDirty = true;
		updateFrameGeometry();
		wl_surface_commit(mObjects->surface);
		mConnection->flush();
	}

	void WaylandWindow::commitFrameGeometry() {
		if (mGeometryDirty) updateFrameGeometry();
		requestFrameCallback();
	}

	void WaylandWindow::requestFrameCallback() {
		if (mObjects->frameCallback) return;
		mObjects->frameCallback = wl_surface_frame(mObjects->surface);
		wl_callback_add_listener(mObjects->frameCallback, &kFrameListener, this);
	}

	bool WaylandWindow::isWaitingForFrame() const { return mObjects->frameCallback != nullptr; }

	void WaylandWindow::updateFrameGeometry() {
		Objects& objects = *mObjects;
		const Vec2F surface = mFrame.surfaceSize();
		const RectF content = mFrame.contentRect();
		const RectF input = mFrame.inputRect();

		xdg_surface_set_window_geometry(
			objects.shellSurface, (int32_t) content.x, (int32_t) content.y, (int32_t) content.z, (int32_t) content.w
		);

		wl_region* inputRegion = wl_compositor_create_region(mConnection->compositor());
		if (mAcceptsInput) {
			wl_region_add(inputRegion, (int32_t) input.x, (int32_t) input.y, (int32_t) input.z, (int32_t) input.w);
		}
		wl_surface_set_input_region(objects.surface, inputRegion);
		wl_region_destroy(inputRegion);

		if (!mFrame.hasShadow()) {
			wl_region* opaqueRegion = wl_compositor_create_region(mConnection->compositor());
			wl_region_add(opaqueRegion, 0, 0, (int32_t) surface.x, (int32_t) surface.y);
			wl_surface_set_opaque_region(objects.surface, opaqueRegion);
			wl_region_destroy(opaqueRegion);
		} else {
			wl_surface_set_opaque_region(objects.surface, nullptr);
		}

		if (objects.viewport) {
			wp_viewport_set_destination(objects.viewport, (int32_t) surface.x, (int32_t) surface.y);
		} else {
			wl_surface_set_buffer_scale(objects.surface, (int32_t) std::lround(mScale));
		}

		mGeometryDirty = false;
	}

	namespace {
		struct ActivationToken {
			std::string value;
			bool done = false;
		};

		void activationTokenDone(void* data, xdg_activation_token_v1*, const char* token) {
			auto& result = *static_cast<ActivationToken*>(data);
			result.value = token ? token : "";
			result.done = true;
		}

		const xdg_activation_token_v1_listener kActivationTokenListener = { activationTokenDone };
	}

	std::string WaylandWindow::activationToken() {
		xdg_activation_v1* activation = mConnection->activation();
		if (!activation) return {};

		wl_display* display = mConnection->display();
		wl_event_queue* queue = wl_display_create_queue(display);
		auto* wrapped = static_cast<xdg_activation_v1*>(wl_proxy_create_wrapper(activation));
		wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(wrapped), queue);

		ActivationToken result;
		xdg_activation_token_v1* token = xdg_activation_v1_get_activation_token(wrapped);
		xdg_activation_token_v1_add_listener(token, &kActivationTokenListener, &result);
		if (mConnection->seat() && mSeat->lastInputSerial() != 0)
			xdg_activation_token_v1_set_serial(token, mSeat->lastInputSerial(), mConnection->seat());
		xdg_activation_token_v1_set_surface(token, mObjects->surface);
		xdg_activation_token_v1_commit(token);
		while (!result.done && wl_display_roundtrip_queue(display, queue) >= 0) {
		}

		xdg_activation_token_v1_destroy(token);
		wl_proxy_wrapper_destroy(wrapped);
		wl_event_queue_destroy(queue);
		return result.value;
	}

	bool WaylandWindow::openUrl(const std::string& url) {
		if (url.empty()) return false;

		std::vector<std::string> environment;
		for (char** entry = environ; *entry; entry++) {
			const std::string variable = *entry;
			if (variable.starts_with("XDG_ACTIVATION_TOKEN=") || variable.starts_with("DESKTOP_STARTUP_ID=")) continue;
			environment.push_back(variable);
		}
		const std::string token = activationToken();
		if (!token.empty()) {
			environment.push_back("XDG_ACTIVATION_TOKEN=" + token);
			environment.push_back("DESKTOP_STARTUP_ID=" + token);
		}

		std::vector<char*> environmentPointers;
		for (std::string& variable : environment)
			environmentPointers.push_back(variable.data());
		environmentPointers.push_back(nullptr);

		std::string program = "xdg-open";
		std::string argument = url;
		char* arguments[] = { program.data(), argument.data(), nullptr };

		pid_t child = 0;
		if (posix_spawnp(&child, program.c_str(), nullptr, nullptr, arguments, environmentPointers.data()) != 0)
			return false;
		std::thread([child]() { waitpid(child, nullptr, 0); }).detach();
		return true;
	}

	Vec2F WaylandWindow::toContent(Vec2F surfacePoint) const {
		const RectF content = mFrame.contentRect();
		return { surfacePoint.x - content.x, surfacePoint.y - content.y };
	}

	void WaylandWindow::onPointerSurfaceMove(Vec2F surfacePoint) {
		const bool contentHoldsPointer = mHeldButtons > 0;
		const WindowEdge edge = contentHoldsPointer ? WindowEdge::None : mFrame.edgeAt(surfacePoint);
		mHoveredEdge = edge;

		if (edge != WindowEdge::None) {
			const halnf outside = WindowInputDispatcher::kPointerOutsideCoordinate;
			mSeat->setCursor(cursorForEdge(edge));
			platformInput().onPointerMoved({ outside, outside });
			return;
		}

		mSeat->setCursor(mClientCursor);
		platformInput().onPointerMoved(toContent(surfacePoint));
	}

	// Touchpad pinches arrive at the pointer; the gesture is placed where the pointer is, in content coordinates.
	void WaylandWindow::onPointerSurfacePinch(const TouchGesture& gesture) {
		TouchGesture content = gesture;
		content.center = toContent(gesture.center);
		platformInput().onGesture(content);
	}

	void WaylandWindow::onPointerSurfaceLeft() {
		mHeldButtons = 0;
		mHoveredEdge = WindowEdge::None;
		platformInput().onPointerLeft();
	}

	void WaylandWindow::onPointerSurfaceButton(uint32_t button, bool pressed, uint32_t serial) {
		if (pressed && mHoveredEdge != WindowEdge::None) {
			if (button == BTN_LEFT)
				xdg_toplevel_resize(mObjects->toplevel, mConnection->seat(), serial, resizeEdgeCode(mHoveredEdge));
			return;
		}

		const std::optional<InputID> id = inputFromPointerButton(button);
		if (!id) return;

		if (pressed) mHeldButtons++;
		else if (mHeldButtons > 0) mHeldButtons--;
		platformInput().onPointerButton(*id, pressed);
	}

	void WaylandWindow::minimize() { xdg_toplevel_set_minimized(mObjects->toplevel); }

	void WaylandWindow::toggleMaximize() {
		if (mMaximized) xdg_toplevel_unset_maximized(mObjects->toplevel);
		else xdg_toplevel_set_maximized(mObjects->toplevel);
	}

	void WaylandWindow::setFullScreen(bool fullScreen) {
		if (fullScreen) xdg_toplevel_set_fullscreen(mObjects->toplevel, nullptr);
		else xdg_toplevel_unset_fullscreen(mObjects->toplevel);
	}

	void WaylandWindow::beginMove() {
		xdg_toplevel_move(mObjects->toplevel, mConnection->seat(), mSeat->lastInputSerial());
	}

	void WaylandWindow::beginResize(WindowEdge edge) {
		if (edge == WindowEdge::None) return;
		xdg_toplevel_resize(mObjects->toplevel, mConnection->seat(), mSeat->lastInputSerial(), resizeEdgeCode(edge));
	}

	void WaylandWindow::showWindowMenu(Vec2F contentPosition) {
		const RectF content = mFrame.contentRect();
		xdg_toplevel_show_window_menu(
			mObjects->toplevel,
			mConnection->seat(),
			mSeat->lastInputSerial(),
			(int32_t) (contentPosition.x + content.x),
			(int32_t) (contentPosition.y + content.y)
		);
	}

	void WaylandWindow::setCursor(CursorShape shape) {
		mClientCursor = shape;
		if (mHoveredEdge == WindowEdge::None) mSeat->setCursor(shape);
	}

	void WaylandWindow::setClipboardText(const std::string& text) { mSeat->setClipboardText(text); }

	std::string WaylandWindow::clipboardText() { return mSeat->clipboardText(); }

	ClipboardImage WaylandWindow::clipboardImage() { return mSeat->clipboardImage(); }

	void WaylandWindow::setTextInputEnabled(bool enabled) { mSeat->setTextInputEnabled(enabled); }

	void WaylandWindow::setTextInputRect(const RectF& contentRect) {
		const RectF content = mFrame.contentRect();
		mSeat->setTextInputRect({ contentRect.x + content.x, contentRect.y + content.y, contentRect.z, contentRect.w });
	}

	PlatformInfo WaylandWindow::platformInfo() const {
		PlatformInfo info;
		info.system = OperatingSystem::Linux;
		info.formFactor = FormFactor::Desktop;
		info.contentScale = mScale;
		info.screenSize = mFrame.contentSize();
		info.hasMouse = mSeat->hasPointer();
		info.hasTouch = mSeat->hasTouch();
		info.hasPen = mSeat->hasTablet();
		info.hasPhysicalKeyboard = mSeat->hasKeyboard();
		info.hasWindowControls = true;
		info.canMoveWindow = true;
		info.canResizeWindow = true;
		info.hasHoverPointer = info.hasMouse || info.hasPen;
		return info;
	}

	std::unique_ptr<PlatformWindow>
	createPlatformWindow(Window& owner, Vec2F contentSize, const WindowIdentity& identity) {
		return std::make_unique<WaylandWindow>(owner, contentSize, identity);
	}
}
