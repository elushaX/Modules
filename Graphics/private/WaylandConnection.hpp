#pragma once

#include <cstdint>
#include <string>

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_seat;
struct wl_data_device_manager;
struct xdg_wm_base;
struct wp_viewporter;
struct wp_fractional_scale_manager_v1;
struct wp_cursor_shape_manager_v1;
struct zwp_pointer_gestures_v1;
struct zwp_text_input_manager_v3;
struct zxdg_decoration_manager_v1;
struct wl_output;
struct zwp_tablet_manager_v2;
struct xdg_activation_v1;

namespace tp {

	class WaylandConnection {
	public:
		WaylandConnection();
		~WaylandConnection();

		WaylandConnection(const WaylandConnection&) = delete;
		WaylandConnection& operator=(const WaylandConnection&) = delete;

		[[nodiscard]] wl_display* display() const { return mDisplay; }
		[[nodiscard]] wl_compositor* compositor() const { return mCompositor; }
		[[nodiscard]] xdg_wm_base* windowManager() const { return mWindowManager; }
		[[nodiscard]] wl_seat* seat() const { return mSeat; }
		[[nodiscard]] wl_seat* bindSeat(const void* listener, void* data);
		[[nodiscard]] wl_data_device_manager* dataDeviceManager() const { return mDataDeviceManager; }
		[[nodiscard]] wp_viewporter* viewporter() const { return mViewporter; }
		[[nodiscard]] wp_fractional_scale_manager_v1* fractionalScaleManager() const { return mFractionalScaleManager; }
		[[nodiscard]] wp_cursor_shape_manager_v1* cursorShapeManager() const { return mCursorShapeManager; }
		[[nodiscard]] zwp_pointer_gestures_v1* pointerGestures() const { return mPointerGestures; }
		[[nodiscard]] zwp_text_input_manager_v3* textInputManager() const { return mTextInputManager; }
		[[nodiscard]] zxdg_decoration_manager_v1* decorationManager() const { return mDecorationManager; }
		[[nodiscard]] zwp_tablet_manager_v2* tabletManager() const { return mTabletManager; }
		[[nodiscard]] xdg_activation_v1* activation() const { return mActivation; }
		[[nodiscard]] int32_t refreshMillihertz() const { return mRefreshMillihertz; }

		bool dispatch(int timeoutMs);
		void roundtrip();
		void flush();

	private:
		friend struct WaylandRegistryListener;

		void bindGlobal(wl_registry* registry, uint32_t name, const std::string& interface, uint32_t version);

		wl_display* mDisplay = nullptr;
		wl_registry* mRegistry = nullptr;
		wl_compositor* mCompositor = nullptr;
		xdg_wm_base* mWindowManager = nullptr;
		wl_seat* mSeat = nullptr;
		uint32_t mSeatName = 0;
		uint32_t mSeatVersion = 0;
		wl_data_device_manager* mDataDeviceManager = nullptr;
		wp_viewporter* mViewporter = nullptr;
		wp_fractional_scale_manager_v1* mFractionalScaleManager = nullptr;
		wp_cursor_shape_manager_v1* mCursorShapeManager = nullptr;
		zwp_pointer_gestures_v1* mPointerGestures = nullptr;
		zwp_text_input_manager_v3* mTextInputManager = nullptr;
		zxdg_decoration_manager_v1* mDecorationManager = nullptr;
		zwp_tablet_manager_v2* mTabletManager = nullptr;
		xdg_activation_v1* mActivation = nullptr;
		wl_output* mOutput = nullptr;
		int32_t mRefreshMillihertz = 60000;
	};
}
