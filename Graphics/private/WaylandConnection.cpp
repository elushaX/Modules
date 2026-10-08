#include "WaylandConnection.hpp"

#include "cursor-shape-v1-client-protocol.h"
#include "pointer-gestures-unstable-v1-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include "tablet-v2-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <wayland-client.h>

#include <algorithm>
#include <cerrno>
#include <poll.h>
#include <stdexcept>

namespace tp {

	struct WaylandRegistryListener {
		static void global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
			static_cast<WaylandConnection*>(data)->bindGlobal(registry, name, interface, version);
		}

		static void globalRemove(void*, wl_registry*, uint32_t) {}

		static void ping(void*, xdg_wm_base* windowManager, uint32_t serial) { xdg_wm_base_pong(windowManager, serial); }

		static void
		outputGeometry(void*, wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {}

		static void outputMode(void* data, wl_output*, uint32_t flags, int32_t, int32_t, int32_t refresh) {
			if (!(flags & WL_OUTPUT_MODE_CURRENT) || refresh <= 0) return;
			static_cast<WaylandConnection*>(data)->mRefreshMillihertz = refresh;
		}

		static void outputDone(void*, wl_output*) {}
		static void outputScale(void*, wl_output*, int32_t) {}
		static void outputName(void*, wl_output*, const char*) {}
		static void outputDescription(void*, wl_output*, const char*) {}
	};

	namespace {
		const wl_registry_listener kRegistryListener = {
			WaylandRegistryListener::global,
			WaylandRegistryListener::globalRemove,
		};

		const xdg_wm_base_listener kWindowManagerListener = { WaylandRegistryListener::ping };

		const wl_output_listener kOutputListener = {
			WaylandRegistryListener::outputGeometry, WaylandRegistryListener::outputMode,
			WaylandRegistryListener::outputDone,     WaylandRegistryListener::outputScale,
			WaylandRegistryListener::outputName,     WaylandRegistryListener::outputDescription,
		};

		template <typename Interface>
		Interface*
		bindAs(wl_registry* registry, uint32_t name, const wl_interface* interface, uint32_t offered, uint32_t wanted) {
			return static_cast<Interface*>(wl_registry_bind(registry, name, interface, std::min(offered, wanted)));
		}
	}

	WaylandConnection::WaylandConnection() {
		mDisplay = wl_display_connect(nullptr);
		if (!mDisplay) throw std::runtime_error("cannot connect to the Wayland display; this build supports Wayland only");

		mRegistry = wl_display_get_registry(mDisplay);
		wl_registry_add_listener(mRegistry, &kRegistryListener, this);
		wl_display_roundtrip(mDisplay);
		wl_display_roundtrip(mDisplay);

		if (!mCompositor || !mWindowManager)
			throw std::runtime_error("Wayland compositor lacks wl_compositor or xdg_wm_base");
	}

	WaylandConnection::~WaylandConnection() {
		if (mDecorationManager) zxdg_decoration_manager_v1_destroy(mDecorationManager);
		if (mTextInputManager) zwp_text_input_manager_v3_destroy(mTextInputManager);
		if (mActivation) xdg_activation_v1_destroy(mActivation);
		if (mTabletManager) zwp_tablet_manager_v2_destroy(mTabletManager);
		if (mCursorShapeManager) wp_cursor_shape_manager_v1_destroy(mCursorShapeManager);
		if (mPointerGestures) zwp_pointer_gestures_v1_destroy(mPointerGestures);
		if (mFractionalScaleManager) wp_fractional_scale_manager_v1_destroy(mFractionalScaleManager);
		if (mViewporter) wp_viewporter_destroy(mViewporter);
		if (mDataDeviceManager) wl_data_device_manager_destroy(mDataDeviceManager);
		if (mOutput) wl_output_release(mOutput);
		if (mSeat) wl_seat_release(mSeat);
		if (mWindowManager) xdg_wm_base_destroy(mWindowManager);
		if (mCompositor) wl_compositor_destroy(mCompositor);
		if (mRegistry) wl_registry_destroy(mRegistry);
		if (mDisplay) {
			wl_display_flush(mDisplay);
			wl_display_disconnect(mDisplay);
		}
	}

	void
	WaylandConnection::bindGlobal(wl_registry* registry, uint32_t name, const std::string& interface, uint32_t version) {
		if (interface == wl_compositor_interface.name) {
			mCompositor = bindAs<wl_compositor>(registry, name, &wl_compositor_interface, version, 4);
		} else if (interface == xdg_wm_base_interface.name) {
			mWindowManager = bindAs<xdg_wm_base>(registry, name, &xdg_wm_base_interface, version, 5);
			xdg_wm_base_add_listener(mWindowManager, &kWindowManagerListener, this);
		} else if (interface == wl_seat_interface.name && mSeatName == 0) {
			mSeatName = name;
			mSeatVersion = version;
		} else if (interface == wl_data_device_manager_interface.name) {
			mDataDeviceManager =
				bindAs<wl_data_device_manager>(registry, name, &wl_data_device_manager_interface, version, 3);
		} else if (interface == wp_viewporter_interface.name) {
			mViewporter = bindAs<wp_viewporter>(registry, name, &wp_viewporter_interface, version, 1);
		} else if (interface == wp_fractional_scale_manager_v1_interface.name) {
			mFractionalScaleManager =
				bindAs<wp_fractional_scale_manager_v1>(registry, name, &wp_fractional_scale_manager_v1_interface, version, 1);
		} else if (interface == zwp_pointer_gestures_v1_interface.name) {
			mPointerGestures =
				bindAs<zwp_pointer_gestures_v1>(registry, name, &zwp_pointer_gestures_v1_interface, version, 1);
		} else if (interface == wp_cursor_shape_manager_v1_interface.name) {
			mCursorShapeManager =
				bindAs<wp_cursor_shape_manager_v1>(registry, name, &wp_cursor_shape_manager_v1_interface, version, 1);
		} else if (interface == zwp_text_input_manager_v3_interface.name) {
			mTextInputManager =
				bindAs<zwp_text_input_manager_v3>(registry, name, &zwp_text_input_manager_v3_interface, version, 1);
		} else if (interface == zxdg_decoration_manager_v1_interface.name) {
			mDecorationManager =
				bindAs<zxdg_decoration_manager_v1>(registry, name, &zxdg_decoration_manager_v1_interface, version, 1);
		} else if (interface == zwp_tablet_manager_v2_interface.name) {
			mTabletManager = bindAs<zwp_tablet_manager_v2>(registry, name, &zwp_tablet_manager_v2_interface, version, 1);
		} else if (interface == xdg_activation_v1_interface.name) {
			mActivation = bindAs<xdg_activation_v1>(registry, name, &xdg_activation_v1_interface, version, 1);
		} else if (interface == wl_output_interface.name && !mOutput) {
			mOutput = bindAs<wl_output>(registry, name, &wl_output_interface, version, 4);
			wl_output_add_listener(mOutput, &kOutputListener, this);
		}
	}

	wl_seat* WaylandConnection::bindSeat(const void* listener, void* data) {
		if (mSeat || mSeatName == 0) return mSeat;

		mSeat = bindAs<wl_seat>(mRegistry, mSeatName, &wl_seat_interface, mSeatVersion, 8);
		wl_seat_add_listener(mSeat, static_cast<const wl_seat_listener*>(listener), data);
		return mSeat;
	}

	bool WaylandConnection::dispatch(int timeoutMs) {
		if (wl_display_dispatch_pending(mDisplay) > 0) {
			flush();
			return true;
		}

		while (wl_display_prepare_read(mDisplay) != 0) {
			if (wl_display_dispatch_pending(mDisplay) > 0) return true;
		}

		flush();

		pollfd descriptor = { wl_display_get_fd(mDisplay), POLLIN, 0 };
		const int ready = poll(&descriptor, 1, timeoutMs);
		if (ready <= 0) {
			wl_display_cancel_read(mDisplay);
			return false;
		}

		if (wl_display_read_events(mDisplay) < 0) throw std::runtime_error("lost the Wayland connection");
		wl_display_dispatch_pending(mDisplay);
		flush();
		return true;
	}

	void WaylandConnection::roundtrip() { wl_display_roundtrip(mDisplay); }

	void WaylandConnection::flush() {
		while (wl_display_flush(mDisplay) < 0 && errno == EAGAIN) {
			pollfd descriptor = { wl_display_get_fd(mDisplay), POLLOUT, 0 };
			poll(&descriptor, 1, -1);
		}
	}
}
