#include "WaylandSeat.hpp"

#include "KeyMapping.hpp"
#include "WaylandConnection.hpp"
#include "WaylandWindow.hpp"
#include "WindowInput.hpp"

#include "cursor-shape-v1-client-protocol.h"
#include "pointer-gestures-unstable-v1-client-protocol.h"
#include "tablet-v2-client-protocol.h"
#include "text-input-unstable-v3-client-protocol.h"

#include <wayland-client.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

#include <algorithm>
#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <memory>
#include <vector>

namespace tp {

	namespace {
		constexpr const char* kTextMimeType = "text/plain;charset=utf-8";
		constexpr const char* kLegacyTextMimeType = "text/plain";
		constexpr const char* kPngMimeType = "image/png";
		constexpr const char* kImageMimePrefix = "image/";
		constexpr int kClipboardReadTimeoutMs = 500;
		constexpr halnf kWheelStep = 10;

		uint32_t cursorShapeCode(CursorShape shape) {
			switch (shape) {
				case CursorShape::Default: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
				case CursorShape::Pointer: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER;
				case CursorShape::Text: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT;
				case CursorShape::Move: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE;
				case CursorShape::Grab: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRAB;
				case CursorShape::Grabbing: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRABBING;
				case CursorShape::ResizeHorizontal: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE;
				case CursorShape::ResizeVertical: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE;
				case CursorShape::ResizeNorthWestSouthEast: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NWSE_RESIZE;
				case CursorShape::ResizeNorthEastSouthWest: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NESW_RESIZE;
				case CursorShape::NotAllowed: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NOT_ALLOWED;
				case CursorShape::Crosshair: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR;
				case CursorShape::Hidden: return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
			}
			return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
		}

		void writeAll(int descriptor, const std::string& text) {
			size_t written = 0;
			while (written < text.size()) {
				const ssize_t result = write(descriptor, text.data() + written, text.size() - written);
				if (result < 0 && errno == EINTR) continue;
				if (result <= 0) return;
				written += (size_t) result;
			}
		}

		std::string readAll(int descriptor, wl_display* display) {
			std::string text;
			char buffer[4096];

			wl_display_flush(display);

			while (true) {
				pollfd readable = { descriptor, POLLIN, 0 };
				if (poll(&readable, 1, kClipboardReadTimeoutMs) <= 0) break;

				const ssize_t result = read(descriptor, buffer, sizeof(buffer));
				if (result < 0 && errno == EINTR) continue;
				if (result <= 0) break;
				text.append(buffer, (size_t) result);
			}

			return text;
		}

		timespec millisecondsToTimespec(int32_t milliseconds) {
			return { milliseconds / 1000, (long) (milliseconds % 1000) * 1000000L };
		}
	}

	class WaylandSeat::Keyboard {
	public:
		Keyboard(WaylandSeat& seat, wl_keyboard* keyboard);
		~Keyboard();

		void onKeymap(uint32_t format, int descriptor, uint32_t size);
		void onEnter(uint32_t serial, wl_array* keys);
		void onLeave();
		void onKey(uint32_t serial, uint32_t key, uint32_t state);
		void onModifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group);
		void onRepeatInfo(int32_t rate, int32_t delay);
		void onRepeatTimer();

		[[nodiscard]] int repeatTimerDescriptor() const { return mRepeatTimer; }

	private:
		void pressKey(uint32_t key);
		void releaseKey(uint32_t key);
		void emitText(uint32_t key);
		void armRepeat(uint32_t key);
		void disarmRepeat();
		[[nodiscard]] xkb_keysym_t composed(xkb_keysym_t symbol);

		WaylandSeat& mSeat;
		wl_keyboard* mKeyboard;
		xkb_context* mContext = nullptr;
		xkb_keymap* mKeymap = nullptr;
		xkb_state* mState = nullptr;
		xkb_compose_table* mComposeTable = nullptr;
		xkb_compose_state* mComposeState = nullptr;
		int mRepeatTimer = -1;
		int32_t mRepeatRate = 25;
		int32_t mRepeatDelay = 600;
		uint32_t mRepeatKey = 0;
	};

	class WaylandSeat::Pointer {
	public:
		Pointer(WaylandSeat& seat, wl_pointer* pointer);
		~Pointer();

		void onEnter(uint32_t serial, wl_fixed_t x, wl_fixed_t y);
		void onLeave();
		void onMotion(wl_fixed_t x, wl_fixed_t y);
		void onButton(uint32_t serial, uint32_t button, uint32_t state);
		void onAxis(uint32_t axis, wl_fixed_t value);
		void onAxisDiscrete(uint32_t axis, int32_t steps);
		void onFrame();

		void setCursor(CursorShape shape);

		void onPinchBegin();
		void onPinchUpdate(wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation);
		void onPinchEnd();

	private:
		void applyCursor();

		WaylandSeat& mSeat;
		wl_pointer* mPointer;
		zwp_pointer_gesture_pinch_v1* mPinch = nullptr;
		PointerPinchConverter mPinchConverter;
		Vec2F mSurfacePosition = { 0, 0 };
		wp_cursor_shape_device_v1* mShapeDevice = nullptr;
		uint32_t mEnterSerial = 0;
		bool mInside = false;
		CursorShape mShape = CursorShape::Default;
		bool mShapeApplied = false;
		Vec2F mPendingScroll = { 0, 0 };
		bool mDiscreteScroll = false;
	};

	class WaylandSeat::Clipboard {
	public:
		Clipboard(WaylandSeat& seat, wl_data_device_manager* manager, wl_seat* waylandSeat);
		~Clipboard();

		void setText(const std::string& text);
		[[nodiscard]] std::string text();
		[[nodiscard]] ClipboardImage image();

		void onOffer(wl_data_offer* offer);
		void onOfferMimeType(wl_data_offer* offer, const char* mimeType);
		void onSelection(wl_data_offer* offer);
		void onSourceSend(const char* mimeType, int descriptor);
		void onSourceCancelled(wl_data_source* source);

	private:
		[[nodiscard]] std::string receive(const char* mimeType);

		WaylandSeat& mSeat;
		wl_data_device_manager* mManager;
		wl_data_device* mDevice = nullptr;
		wl_data_offer* mPendingOffer = nullptr;
		std::vector<std::string> mPendingOfferTypes;
		wl_data_offer* mSelection = nullptr;
		std::vector<std::string> mSelectionTypes;
		wl_data_source* mSource = nullptr;
		std::string mOwnedText;
	};

	class WaylandSeat::TextInput {
	public:
		TextInput(WaylandSeat& seat, zwp_text_input_manager_v3* manager, wl_seat* waylandSeat);
		~TextInput();

		void setEnabled(bool enabled);
		void setCursorRect(const RectF& surfaceRect);

		void onEnter();
		void onLeave();
		void onCommitString(const char* text);
		void onDone();

	private:
		void sendState();

		WaylandSeat& mSeat;
		zwp_text_input_v3* mTextInput = nullptr;
		bool mFocused = false;
		bool mEnabled = false;
		RectF mCursorRect = { 0, 0, 1, 1 };
		std::string mPendingCommit;
	};

	class WaylandSeat::Tablet {
	public:
		Tablet(WaylandSeat& seat, zwp_tablet_manager_v2* manager, wl_seat* waylandSeat);
		~Tablet();

		void onTabletAdded(zwp_tablet_v2* tablet);
		void onToolAdded(zwp_tablet_tool_v2* tool);
		void onPadAdded(zwp_tablet_pad_v2* pad);

		struct Tool {
			Tablet& owner;
			zwp_tablet_tool_v2* handle = nullptr;
			wp_cursor_shape_device_v1* shapeDevice = nullptr;
			PointerSample sample;
			PointerTool kind = PointerTool::Pen;
			Vec2F position = { 0, 0 };
			bool inside = false;
			bool moved = false;
			int contactChange = 0;
			std::vector<std::pair<InputID, bool>> buttons;
		};

		void onToolType(Tool& tool, uint32_t type);
		void onProximityIn(Tool& tool, uint32_t serial);
		void onProximityOut(Tool& tool);
		void onFrame(Tool& tool, uint32_t time);
		void onToolRemoved(Tool& tool);
		void recordSerial(uint32_t serial) { mSeat.recordSerial(serial); }

	private:
		WaylandSeat& mSeat;
		zwp_tablet_seat_v2* mTabletSeat = nullptr;
		std::vector<zwp_tablet_v2*> mTablets;
		std::vector<zwp_tablet_pad_v2*> mPads;
		std::vector<std::unique_ptr<Tool>> mTools;
	};

	struct WaylandSeatListeners {
		static void capabilities(void* data, wl_seat*, uint32_t capabilities) {
			static_cast<WaylandSeat*>(data)->onCapabilities(capabilities);
		}

		static void name(void*, wl_seat*, const char*) {}

		static WaylandSeat::Keyboard* keyboard(void* data) { return static_cast<WaylandSeat::Keyboard*>(data); }
		static WaylandSeat::Pointer* pointer(void* data) { return static_cast<WaylandSeat::Pointer*>(data); }
		static WaylandSeat::Clipboard* clipboard(void* data) { return static_cast<WaylandSeat::Clipboard*>(data); }
		static WaylandSeat::TextInput* textInput(void* data) { return static_cast<WaylandSeat::TextInput*>(data); }

		static void keymap(void* data, wl_keyboard*, uint32_t format, int32_t descriptor, uint32_t size) {
			keyboard(data)->onKeymap(format, descriptor, size);
		}
		static void keyEnter(void* data, wl_keyboard*, uint32_t serial, wl_surface*, wl_array* keys) {
			keyboard(data)->onEnter(serial, keys);
		}
		static void keyLeave(void* data, wl_keyboard*, uint32_t, wl_surface*) { keyboard(data)->onLeave(); }
		static void key(void* data, wl_keyboard*, uint32_t serial, uint32_t, uint32_t key, uint32_t state) {
			keyboard(data)->onKey(serial, key, state);
		}
		static void modifiers(
			void* data, wl_keyboard*, uint32_t, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group
		) {
			keyboard(data)->onModifiers(depressed, latched, locked, group);
		}
		static void repeatInfo(void* data, wl_keyboard*, int32_t rate, int32_t delay) {
			keyboard(data)->onRepeatInfo(rate, delay);
		}

		static void pointerEnter(void* data, wl_pointer*, uint32_t serial, wl_surface*, wl_fixed_t x, wl_fixed_t y) {
			pointer(data)->onEnter(serial, x, y);
		}
		static void pointerLeave(void* data, wl_pointer*, uint32_t, wl_surface*) { pointer(data)->onLeave(); }
		static void pointerMotion(void* data, wl_pointer*, uint32_t, wl_fixed_t x, wl_fixed_t y) {
			pointer(data)->onMotion(x, y);
		}
		static void pointerButton(void* data, wl_pointer*, uint32_t serial, uint32_t, uint32_t button, uint32_t state) {
			pointer(data)->onButton(serial, button, state);
		}
		static void pointerAxis(void* data, wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value) {
			pointer(data)->onAxis(axis, value);
		}
		static void pointerFrame(void* data, wl_pointer*) { pointer(data)->onFrame(); }
		static void pointerAxisSource(void*, wl_pointer*, uint32_t) {}
		static void pointerAxisStop(void*, wl_pointer*, uint32_t, uint32_t) {}
		static void pointerAxisDiscrete(void* data, wl_pointer*, uint32_t axis, int32_t steps) {
			pointer(data)->onAxisDiscrete(axis, steps);
		}
		static void pointerAxisValue120(void*, wl_pointer*, uint32_t, int32_t) {}
		static void pointerAxisRelativeDirection(void*, wl_pointer*, uint32_t, uint32_t) {}

		static void pinchBegin(void* data, zwp_pointer_gesture_pinch_v1*, uint32_t, uint32_t, wl_surface*, uint32_t) {
			pointer(data)->onPinchBegin();
		}
		static void pinchUpdate(
			void* data,
			zwp_pointer_gesture_pinch_v1*,
			uint32_t,
			wl_fixed_t dx,
			wl_fixed_t dy,
			wl_fixed_t scale,
			wl_fixed_t rotation
		) {
			pointer(data)->onPinchUpdate(dx, dy, scale, rotation);
		}
		static void pinchEnd(void* data, zwp_pointer_gesture_pinch_v1*, uint32_t, uint32_t, int32_t) {
			pointer(data)->onPinchEnd();
		}

		static void offerMimeType(void* data, wl_data_offer* offer, const char* mimeType) {
			clipboard(data)->onOfferMimeType(offer, mimeType);
		}
		static void offerSourceActions(void*, wl_data_offer*, uint32_t) {}
		static void offerAction(void*, wl_data_offer*, uint32_t) {}

		static void dataOffer(void* data, wl_data_device*, wl_data_offer* offer) { clipboard(data)->onOffer(offer); }
		static void dataEnter(void*, wl_data_device*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t, wl_data_offer* offer) {
			if (offer) wl_data_offer_destroy(offer);
		}
		static void dataLeave(void*, wl_data_device*) {}
		static void dataMotion(void*, wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t) {}
		static void dataDrop(void*, wl_data_device*) {}
		static void dataSelection(void* data, wl_data_device*, wl_data_offer* offer) {
			clipboard(data)->onSelection(offer);
		}

		static void sourceTarget(void*, wl_data_source*, const char*) {}
		static void sourceSend(void* data, wl_data_source*, const char* mimeType, int32_t descriptor) {
			clipboard(data)->onSourceSend(mimeType, descriptor);
		}
		static void sourceCancelled(void* data, wl_data_source* source) { clipboard(data)->onSourceCancelled(source); }
		static void sourceDropPerformed(void*, wl_data_source*) {}
		static void sourceDndFinished(void*, wl_data_source*) {}
		static void sourceAction(void*, wl_data_source*, uint32_t) {}

		static void textEnter(void* data, zwp_text_input_v3*, wl_surface*) { textInput(data)->onEnter(); }
		static void textLeave(void* data, zwp_text_input_v3*, wl_surface*) { textInput(data)->onLeave(); }
		static void textPreedit(void*, zwp_text_input_v3*, const char*, int32_t, int32_t) {}
		static void textCommit(void* data, zwp_text_input_v3*, const char* text) { textInput(data)->onCommitString(text); }
		static void textDeleteSurrounding(void*, zwp_text_input_v3*, uint32_t, uint32_t) {}
		static void textDone(void* data, zwp_text_input_v3*, uint32_t) { textInput(data)->onDone(); }

		static WaylandSeat::Tablet* tablet(void* data) { return static_cast<WaylandSeat::Tablet*>(data); }
		static WaylandSeat::Tablet::Tool& tool(void* data) { return *static_cast<WaylandSeat::Tablet::Tool*>(data); }

		static void tabletAdded(void* data, zwp_tablet_seat_v2*, zwp_tablet_v2* id) { tablet(data)->onTabletAdded(id); }
		static void toolAdded(void* data, zwp_tablet_seat_v2*, zwp_tablet_tool_v2* id) { tablet(data)->onToolAdded(id); }
		static void padAdded(void* data, zwp_tablet_seat_v2*, zwp_tablet_pad_v2* id) { tablet(data)->onPadAdded(id); }

		static void tabletName(void*, zwp_tablet_v2*, const char*) {}
		static void tabletId(void*, zwp_tablet_v2*, uint32_t, uint32_t) {}
		static void tabletPath(void*, zwp_tablet_v2*, const char*) {}
		static void tabletDone(void*, zwp_tablet_v2*) {}
		static void tabletRemoved(void*, zwp_tablet_v2*) {}
		static void tabletBustype(void*, zwp_tablet_v2*, uint32_t) {}

		static void toolType(void* data, zwp_tablet_tool_v2*, uint32_t type) {
			tool(data).owner.onToolType(tool(data), type);
		}
		static void toolSerial(void*, zwp_tablet_tool_v2*, uint32_t, uint32_t) {}
		static void toolWacomId(void*, zwp_tablet_tool_v2*, uint32_t, uint32_t) {}
		static void toolCapability(void*, zwp_tablet_tool_v2*, uint32_t) {}
		static void toolDone(void*, zwp_tablet_tool_v2*) {}
		static void toolRemoved(void* data, zwp_tablet_tool_v2*) { tool(data).owner.onToolRemoved(tool(data)); }
		static void toolProximityIn(void* data, zwp_tablet_tool_v2*, uint32_t serial, zwp_tablet_v2*, wl_surface*) {
			tool(data).owner.onProximityIn(tool(data), serial);
		}
		static void toolProximityOut(void* data, zwp_tablet_tool_v2*) { tool(data).owner.onProximityOut(tool(data)); }
		static void toolDown(void* data, zwp_tablet_tool_v2*, uint32_t serial) {
			tool(data).owner.recordSerial(serial);
			tool(data).contactChange = 1;
		}
		static void toolUp(void* data, zwp_tablet_tool_v2*) { tool(data).contactChange = -1; }
		static void toolMotion(void* data, zwp_tablet_tool_v2*, wl_fixed_t x, wl_fixed_t y) {
			tool(data).position = { (halnf) wl_fixed_to_double(x), (halnf) wl_fixed_to_double(y) };
			tool(data).moved = true;
		}
		static void toolPressure(void* data, zwp_tablet_tool_v2*, uint32_t pressure) {
			tool(data).sample.pressure = (halnf) pressure / 65535.0f;
		}
		static void toolDistance(void* data, zwp_tablet_tool_v2*, uint32_t distance) {
			tool(data).sample.distance = (halnf) distance / 65535.0f;
		}
		static void toolTilt(void* data, zwp_tablet_tool_v2*, wl_fixed_t x, wl_fixed_t y) {
			tool(data).sample.tilt = { (halnf) wl_fixed_to_double(x), (halnf) wl_fixed_to_double(y) };
		}
		static void toolRotation(void* data, zwp_tablet_tool_v2*, wl_fixed_t degrees) {
			tool(data).sample.rotation = (halnf) wl_fixed_to_double(degrees);
		}
		static void toolSlider(void*, zwp_tablet_tool_v2*, int32_t) {}
		static void toolWheel(void*, zwp_tablet_tool_v2*, wl_fixed_t, int32_t) {}
		static void toolButton(void* data, zwp_tablet_tool_v2*, uint32_t serial, uint32_t button, uint32_t state) {
			tool(data).owner.recordSerial(serial);
			const bool pressed = state == ZWP_TABLET_TOOL_V2_BUTTON_STATE_PRESSED;
			if (button == BTN_STYLUS) tool(data).buttons.push_back({ InputID::MOUSE2, pressed });
			else if (button == BTN_STYLUS2) tool(data).buttons.push_back({ InputID::MOUSE3, pressed });
		}
		static void toolFrame(void* data, zwp_tablet_tool_v2*, uint32_t time) {
			tool(data).owner.onFrame(tool(data), time);
		}

		static void padGroup(void*, zwp_tablet_pad_v2*, zwp_tablet_pad_group_v2*) {}
		static void padPath(void*, zwp_tablet_pad_v2*, const char*) {}
		static void padButtons(void*, zwp_tablet_pad_v2*, uint32_t) {}
		static void padDone(void*, zwp_tablet_pad_v2*) {}
		static void padButton(void*, zwp_tablet_pad_v2*, uint32_t, uint32_t, uint32_t) {}
		static void padEnter(void*, zwp_tablet_pad_v2*, uint32_t, zwp_tablet_v2*, wl_surface*) {}
		static void padLeave(void*, zwp_tablet_pad_v2*, uint32_t, wl_surface*) {}
		static void padRemoved(void*, zwp_tablet_pad_v2*) {}
	};

	namespace {
		const wl_seat_listener kSeatListener = { WaylandSeatListeners::capabilities, WaylandSeatListeners::name };

		const wl_keyboard_listener kKeyboardListener = {
			WaylandSeatListeners::keymap, WaylandSeatListeners::keyEnter,  WaylandSeatListeners::keyLeave,
			WaylandSeatListeners::key,    WaylandSeatListeners::modifiers, WaylandSeatListeners::repeatInfo,
		};

		const wl_pointer_listener kPointerListener = {
			WaylandSeatListeners::pointerEnter,
			WaylandSeatListeners::pointerLeave,
			WaylandSeatListeners::pointerMotion,
			WaylandSeatListeners::pointerButton,
			WaylandSeatListeners::pointerAxis,
			WaylandSeatListeners::pointerFrame,
			WaylandSeatListeners::pointerAxisSource,
			WaylandSeatListeners::pointerAxisStop,
			WaylandSeatListeners::pointerAxisDiscrete,
			WaylandSeatListeners::pointerAxisValue120,
			WaylandSeatListeners::pointerAxisRelativeDirection,
		};

		const zwp_pointer_gesture_pinch_v1_listener kPinchListener = {
			WaylandSeatListeners::pinchBegin,
			WaylandSeatListeners::pinchUpdate,
			WaylandSeatListeners::pinchEnd,
		};

		const wl_data_offer_listener kOfferListener = {
			WaylandSeatListeners::offerMimeType,
			WaylandSeatListeners::offerSourceActions,
			WaylandSeatListeners::offerAction,
		};

		const wl_data_device_listener kDataDeviceListener = {
			WaylandSeatListeners::dataOffer,  WaylandSeatListeners::dataEnter, WaylandSeatListeners::dataLeave,
			WaylandSeatListeners::dataMotion, WaylandSeatListeners::dataDrop,  WaylandSeatListeners::dataSelection,
		};

		const wl_data_source_listener kSourceListener = {
			WaylandSeatListeners::sourceTarget,      WaylandSeatListeners::sourceSend,
			WaylandSeatListeners::sourceCancelled,   WaylandSeatListeners::sourceDropPerformed,
			WaylandSeatListeners::sourceDndFinished, WaylandSeatListeners::sourceAction,
		};

		const zwp_text_input_v3_listener kTextInputListener = {
			WaylandSeatListeners::textEnter,
			WaylandSeatListeners::textLeave,
			WaylandSeatListeners::textPreedit,
			WaylandSeatListeners::textCommit,
			WaylandSeatListeners::textDeleteSurrounding,
			WaylandSeatListeners::textDone,
		};
		const zwp_tablet_seat_v2_listener kTabletSeatListener = {
			WaylandSeatListeners::tabletAdded,
			WaylandSeatListeners::toolAdded,
			WaylandSeatListeners::padAdded,
		};

		const zwp_tablet_v2_listener kTabletListener = {
			WaylandSeatListeners::tabletName, WaylandSeatListeners::tabletId,      WaylandSeatListeners::tabletPath,
			WaylandSeatListeners::tabletDone, WaylandSeatListeners::tabletRemoved, WaylandSeatListeners::tabletBustype,
		};

		const zwp_tablet_tool_v2_listener kToolListener = {
			WaylandSeatListeners::toolType,        WaylandSeatListeners::toolSerial,       WaylandSeatListeners::toolWacomId,
			WaylandSeatListeners::toolCapability,  WaylandSeatListeners::toolDone,         WaylandSeatListeners::toolRemoved,
			WaylandSeatListeners::toolProximityIn, WaylandSeatListeners::toolProximityOut, WaylandSeatListeners::toolDown,
			WaylandSeatListeners::toolUp,          WaylandSeatListeners::toolMotion,       WaylandSeatListeners::toolPressure,
			WaylandSeatListeners::toolDistance,    WaylandSeatListeners::toolTilt,         WaylandSeatListeners::toolRotation,
			WaylandSeatListeners::toolSlider,      WaylandSeatListeners::toolWheel,        WaylandSeatListeners::toolButton,
			WaylandSeatListeners::toolFrame,
		};

		const zwp_tablet_pad_v2_listener kPadListener = {
			WaylandSeatListeners::padGroup, WaylandSeatListeners::padPath,    WaylandSeatListeners::padButtons,
			WaylandSeatListeners::padDone,  WaylandSeatListeners::padButton,  WaylandSeatListeners::padEnter,
			WaylandSeatListeners::padLeave, WaylandSeatListeners::padRemoved,
		};
	}

	WindowInputObserver& WaylandSeat::input() { return mWindow.platformInput(); }

	WaylandSeat::WaylandSeat(WaylandConnection& connection, WaylandWindow& window) :
		mConnection(connection),
		mWindow(window) {
		if (!connection.bindSeat(&kSeatListener, this)) return;

		if (connection.dataDeviceManager())
			mClipboard = std::make_unique<Clipboard>(*this, connection.dataDeviceManager(), connection.seat());
		if (connection.textInputManager())
			mTextInput = std::make_unique<TextInput>(*this, connection.textInputManager(), connection.seat());
		if (connection.tabletManager())
			mTablet = std::make_unique<Tablet>(*this, connection.tabletManager(), connection.seat());
	}

	WaylandSeat::~WaylandSeat() = default;

	void WaylandSeat::onCapabilities(uint32_t capabilities) {
		const bool hasKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
		const bool hasPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
		mHasTouch = capabilities & WL_SEAT_CAPABILITY_TOUCH;

		if (hasKeyboard && !mKeyboard)
			mKeyboard = std::make_unique<Keyboard>(*this, wl_seat_get_keyboard(mConnection.seat()));
		if (!hasKeyboard) mKeyboard.reset();

		if (hasPointer && !mPointer) mPointer = std::make_unique<Pointer>(*this, wl_seat_get_pointer(mConnection.seat()));
		if (!hasPointer) mPointer.reset();
	}

	int WaylandSeat::repeatTimerDescriptor() const { return mKeyboard ? mKeyboard->repeatTimerDescriptor() : -1; }

	void WaylandSeat::onRepeatTimer() {
		if (mKeyboard) mKeyboard->onRepeatTimer();
	}

	void WaylandSeat::setCursor(CursorShape shape) {
		if (mPointer) mPointer->setCursor(shape);
	}

	void WaylandSeat::setClipboardText(const std::string& text) {
		if (mClipboard) mClipboard->setText(text);
	}

	std::string WaylandSeat::clipboardText() { return mClipboard ? mClipboard->text() : std::string(); }

	ClipboardImage WaylandSeat::clipboardImage() { return mClipboard ? mClipboard->image() : ClipboardImage{}; }

	void WaylandSeat::setTextInputEnabled(bool enabled) {
		if (mTextInput) mTextInput->setEnabled(enabled);
	}

	void WaylandSeat::setTextInputRect(const RectF& surfaceRect) {
		if (mTextInput) mTextInput->setCursorRect(surfaceRect);
	}

	WaylandSeat::Keyboard::Keyboard(WaylandSeat& seat, wl_keyboard* keyboard) :
		mSeat(seat),
		mKeyboard(keyboard) {
		mContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
		mRepeatTimer = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

		const char* locale = std::getenv("LC_ALL");
		if (!locale || !*locale) locale = std::getenv("LC_CTYPE");
		if (!locale || !*locale) locale = std::getenv("LANG");
		if (!locale || !*locale) locale = "C";
		mComposeTable = xkb_compose_table_new_from_locale(mContext, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
		if (mComposeTable) mComposeState = xkb_compose_state_new(mComposeTable, XKB_COMPOSE_STATE_NO_FLAGS);

		wl_keyboard_add_listener(mKeyboard, &kKeyboardListener, this);
	}

	WaylandSeat::Keyboard::~Keyboard() {
		if (mRepeatTimer >= 0) close(mRepeatTimer);
		if (mComposeState) xkb_compose_state_unref(mComposeState);
		if (mComposeTable) xkb_compose_table_unref(mComposeTable);
		if (mState) xkb_state_unref(mState);
		if (mKeymap) xkb_keymap_unref(mKeymap);
		if (mContext) xkb_context_unref(mContext);
		wl_keyboard_release(mKeyboard);
	}

	void WaylandSeat::Keyboard::onKeymap(uint32_t format, int descriptor, uint32_t size) {
		if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
			close(descriptor);
			return;
		}

		void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, descriptor, 0);
		close(descriptor);
		if (mapped == MAP_FAILED) return;

		xkb_keymap* keymap = xkb_keymap_new_from_string(
			mContext, static_cast<const char*>(mapped), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS
		);
		munmap(mapped, size);
		if (!keymap) return;

		if (mState) xkb_state_unref(mState);
		if (mKeymap) xkb_keymap_unref(mKeymap);
		mKeymap = keymap;
		mState = xkb_state_new(mKeymap);
	}

	void WaylandSeat::Keyboard::onEnter(uint32_t serial, wl_array* keys) {
		mSeat.recordSerial(serial);
		mSeat.input().onKeyboardFocus(true);

		const auto* pressed = static_cast<const uint32_t*>(keys->data);
		const size_t count = keys->size / sizeof(uint32_t);
		for (size_t index = 0; index < count; index++) {
			const std::optional<InputID> id = inputFromEvdevKey(pressed[index]);
			if (id) mSeat.input().onKey(*id, true);
		}
	}

	void WaylandSeat::Keyboard::onLeave() {
		disarmRepeat();
		if (mComposeState) xkb_compose_state_reset(mComposeState);
		mSeat.input().onKeyboardFocus(false);
	}

	void WaylandSeat::Keyboard::onKey(uint32_t serial, uint32_t key, uint32_t state) {
		mSeat.recordSerial(serial);

		if (state == WL_KEYBOARD_KEY_STATE_PRESSED) pressKey(key);
		else releaseKey(key);
	}

	void WaylandSeat::Keyboard::pressKey(uint32_t key) {
		const std::optional<InputID> id = inputFromEvdevKey(key);
		if (id) mSeat.input().onKey(*id, true);

		emitText(key);

		if (mKeymap && xkb_keymap_key_repeats(mKeymap, key + 8)) armRepeat(key);
	}

	void WaylandSeat::Keyboard::releaseKey(uint32_t key) {
		if (key == mRepeatKey) disarmRepeat();

		const std::optional<InputID> id = inputFromEvdevKey(key);
		if (id) mSeat.input().onKey(*id, false);
	}

	void WaylandSeat::Keyboard::emitText(uint32_t key) {
		if (!mState) return;

		const xkb_keysym_t symbol = composed(xkb_state_key_get_one_sym(mState, key + 8));
		if (symbol == XKB_KEY_NoSymbol) return;

		char utf8[64] = {};
		if (mComposeState && xkb_compose_state_get_status(mComposeState) == XKB_COMPOSE_COMPOSED) {
			xkb_compose_state_get_utf8(mComposeState, utf8, sizeof(utf8));
			xkb_compose_state_reset(mComposeState);
		} else {
			xkb_state_key_get_utf8(mState, key + 8, utf8, sizeof(utf8));
		}

		const unsigned char first = (unsigned char) utf8[0];
		if (first == 0 || first < 0x20 || first == 0x7f) return;
		mSeat.input().onText(utf8);
	}

	xkb_keysym_t WaylandSeat::Keyboard::composed(xkb_keysym_t symbol) {
		if (!mComposeState || symbol == XKB_KEY_NoSymbol) return symbol;
		if (xkb_compose_state_feed(mComposeState, symbol) != XKB_COMPOSE_FEED_ACCEPTED) return symbol;

		switch (xkb_compose_state_get_status(mComposeState)) {
			case XKB_COMPOSE_COMPOSING: return XKB_KEY_NoSymbol;
			case XKB_COMPOSE_CANCELLED: xkb_compose_state_reset(mComposeState); return XKB_KEY_NoSymbol;
			case XKB_COMPOSE_COMPOSED: return xkb_compose_state_get_one_sym(mComposeState);
			case XKB_COMPOSE_NOTHING: return symbol;
		}
		return symbol;
	}

	void WaylandSeat::Keyboard::onModifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
		if (mState) xkb_state_update_mask(mState, depressed, latched, locked, 0, 0, group);
	}

	void WaylandSeat::Keyboard::onRepeatInfo(int32_t rate, int32_t delay) {
		mRepeatRate = rate;
		mRepeatDelay = delay;
	}

	void WaylandSeat::Keyboard::armRepeat(uint32_t key) {
		if (mRepeatTimer < 0 || mRepeatRate <= 0) return;

		mRepeatKey = key;
		itimerspec timer = {};
		timer.it_value = millisecondsToTimespec(mRepeatDelay);
		timer.it_interval = millisecondsToTimespec(1000 / mRepeatRate);
		timerfd_settime(mRepeatTimer, 0, &timer, nullptr);
	}

	void WaylandSeat::Keyboard::disarmRepeat() {
		mRepeatKey = 0;
		if (mRepeatTimer < 0) return;

		itimerspec timer = {};
		timerfd_settime(mRepeatTimer, 0, &timer, nullptr);
	}

	void WaylandSeat::Keyboard::onRepeatTimer() {
		uint64_t expirations = 0;
		if (read(mRepeatTimer, &expirations, sizeof(expirations)) != sizeof(expirations)) return;
		if (mRepeatKey == 0) return;

		const std::optional<InputID> id = inputFromEvdevKey(mRepeatKey);
		for (uint64_t repeat = 0; repeat < expirations; repeat++) {
			if (id) mSeat.input().onKeyRepeat(*id);
			emitText(mRepeatKey);
		}
	}

	WaylandSeat::Pointer::Pointer(WaylandSeat& seat, wl_pointer* pointer) :
		mSeat(seat),
		mPointer(pointer) {
		wl_pointer_add_listener(mPointer, &kPointerListener, this);

		wp_cursor_shape_manager_v1* manager = seat.mConnection.cursorShapeManager();
		if (manager) mShapeDevice = wp_cursor_shape_manager_v1_get_pointer(manager, mPointer);

		zwp_pointer_gestures_v1* gestures = seat.mConnection.pointerGestures();
		if (gestures) {
			mPinch = zwp_pointer_gestures_v1_get_pinch_gesture(gestures, mPointer);
			zwp_pointer_gesture_pinch_v1_add_listener(mPinch, &kPinchListener, this);
		}
	}

	WaylandSeat::Pointer::~Pointer() {
		if (mPinch) zwp_pointer_gesture_pinch_v1_destroy(mPinch);
		if (mShapeDevice) wp_cursor_shape_device_v1_destroy(mShapeDevice);
		wl_pointer_release(mPointer);
	}

	void WaylandSeat::Pointer::onPinchBegin() {
		mSeat.mWindow.onPointerSurfacePinch(mPinchConverter.begin(mSurfacePosition));
	}

	void WaylandSeat::Pointer::onPinchUpdate(wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation) {
		const Vec2F pan = { (halnf) wl_fixed_to_double(dx), (halnf) wl_fixed_to_double(dy) };
		const std::optional<TouchGesture> gesture = mPinchConverter.update(
			mSurfacePosition, pan, (halnf) wl_fixed_to_double(scale), (halnf) wl_fixed_to_double(rotation)
		);
		if (gesture) mSeat.mWindow.onPointerSurfacePinch(*gesture);
	}

	void WaylandSeat::Pointer::onPinchEnd() {
		if (const std::optional<TouchGesture> gesture = mPinchConverter.end(mSurfacePosition))
			mSeat.mWindow.onPointerSurfacePinch(*gesture);
	}

	void WaylandSeat::Pointer::onEnter(uint32_t serial, wl_fixed_t x, wl_fixed_t y) {
		mEnterSerial = serial;
		mInside = true;
		mSurfacePosition = { (halnf) wl_fixed_to_double(x), (halnf) wl_fixed_to_double(y) };
		mShapeApplied = false;
		mSeat.mWindow.onPointerSurfaceMove({ (halnf) wl_fixed_to_double(x), (halnf) wl_fixed_to_double(y) });
		applyCursor();
	}

	void WaylandSeat::Pointer::onLeave() {
		mInside = false;
		mSeat.mWindow.onPointerSurfaceLeft();
	}

	void WaylandSeat::Pointer::onMotion(wl_fixed_t x, wl_fixed_t y) {
		mSurfacePosition = { (halnf) wl_fixed_to_double(x), (halnf) wl_fixed_to_double(y) };
		mSeat.input().onPointerSample(PointerSample{});
		mSeat.mWindow.onPointerSurfaceMove(mSurfacePosition);
	}

	void WaylandSeat::Pointer::onButton(uint32_t serial, uint32_t button, uint32_t state) {
		mSeat.recordSerial(serial);
		mSeat.input().onPointerSample(PointerSample{});
		mSeat.mWindow.onPointerSurfaceButton(button, state == WL_POINTER_BUTTON_STATE_PRESSED, serial);
	}

	void WaylandSeat::Pointer::onAxis(uint32_t axis, wl_fixed_t value) {
		if (mDiscreteScroll) return;

		const halnf amount = (halnf) -wl_fixed_to_double(value) / kWheelStep;
		if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) mPendingScroll.y += amount;
		else mPendingScroll.x += amount;
	}

	void WaylandSeat::Pointer::onAxisDiscrete(uint32_t axis, int32_t steps) {
		mDiscreteScroll = true;
		if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) mPendingScroll.y -= (halnf) steps;
		else mPendingScroll.x -= (halnf) steps;
	}

	void WaylandSeat::Pointer::onFrame() {
		if (mPendingScroll.x != 0 || mPendingScroll.y != 0) mSeat.input().onScrolled(mPendingScroll);
		mPendingScroll = { 0, 0 };
		mDiscreteScroll = false;
	}

	void WaylandSeat::Pointer::setCursor(CursorShape shape) {
		if (shape == mShape && mShapeApplied) return;

		mShape = shape;
		mShapeApplied = false;
		applyCursor();
	}

	void WaylandSeat::Pointer::applyCursor() {
		if (!mInside || mShapeApplied) return;
		if (mShape == CursorShape::Hidden) {
			wl_pointer_set_cursor(mPointer, mEnterSerial, nullptr, 0, 0);
			mShapeApplied = true;
			return;
		}
		if (!mShapeDevice) return;

		wp_cursor_shape_device_v1_set_shape(mShapeDevice, mEnterSerial, cursorShapeCode(mShape));
		mShapeApplied = true;
	}

	WaylandSeat::Tablet::Tablet(WaylandSeat& seat, zwp_tablet_manager_v2* manager, wl_seat* waylandSeat) :
		mSeat(seat) {
		mTabletSeat = zwp_tablet_manager_v2_get_tablet_seat(manager, waylandSeat);
		zwp_tablet_seat_v2_add_listener(mTabletSeat, &kTabletSeatListener, this);
	}

	WaylandSeat::Tablet::~Tablet() {
		for (const auto& tool : mTools) {
			if (tool->shapeDevice) wp_cursor_shape_device_v1_destroy(tool->shapeDevice);
			zwp_tablet_tool_v2_destroy(tool->handle);
		}
		for (zwp_tablet_pad_v2* pad : mPads)
			zwp_tablet_pad_v2_destroy(pad);
		for (zwp_tablet_v2* tablet : mTablets)
			zwp_tablet_v2_destroy(tablet);
		zwp_tablet_seat_v2_destroy(mTabletSeat);
	}

	void WaylandSeat::Tablet::onTabletAdded(zwp_tablet_v2* tablet) {
		mTablets.push_back(tablet);
		zwp_tablet_v2_add_listener(tablet, &kTabletListener, this);
	}

	void WaylandSeat::Tablet::onPadAdded(zwp_tablet_pad_v2* pad) {
		mPads.push_back(pad);
		zwp_tablet_pad_v2_add_listener(pad, &kPadListener, this);
	}

	void WaylandSeat::Tablet::onToolAdded(zwp_tablet_tool_v2* handle) {
		mTools.push_back(std::make_unique<Tool>(Tool{ *this, handle }));
		Tool& tool = *mTools.back();
		zwp_tablet_tool_v2_add_listener(handle, &kToolListener, &tool);

		wp_cursor_shape_manager_v1* manager = mSeat.mConnection.cursorShapeManager();
		if (manager) tool.shapeDevice = wp_cursor_shape_manager_v1_get_tablet_tool_v2(manager, handle);
	}

	void WaylandSeat::Tablet::onToolRemoved(Tool& tool) {
		if (tool.inside) onProximityOut(tool);
		if (tool.shapeDevice) wp_cursor_shape_device_v1_destroy(tool.shapeDevice);
		zwp_tablet_tool_v2_destroy(tool.handle);
		std::erase_if(mTools, [&tool](const std::unique_ptr<Tool>& entry) { return entry.get() == &tool; });
	}

	void WaylandSeat::Tablet::onToolType(Tool& tool, uint32_t type) {
		switch (type) {
			case ZWP_TABLET_TOOL_V2_TYPE_ERASER: tool.kind = PointerTool::Eraser; break;
			case ZWP_TABLET_TOOL_V2_TYPE_BRUSH: tool.kind = PointerTool::Brush; break;
			case ZWP_TABLET_TOOL_V2_TYPE_PENCIL: tool.kind = PointerTool::Pencil; break;
			case ZWP_TABLET_TOOL_V2_TYPE_AIRBRUSH: tool.kind = PointerTool::Airbrush; break;
			case ZWP_TABLET_TOOL_V2_TYPE_FINGER: tool.kind = PointerTool::Finger; break;
			case ZWP_TABLET_TOOL_V2_TYPE_MOUSE: tool.kind = PointerTool::Mouse; break;
			case ZWP_TABLET_TOOL_V2_TYPE_LENS: tool.kind = PointerTool::Lens; break;
			default: tool.kind = PointerTool::Pen; break;
		}
		tool.sample.tool = tool.kind;
	}

	void WaylandSeat::Tablet::onProximityIn(Tool& tool, uint32_t serial) {
		tool.inside = true;
		tool.sample = PointerSample{ tool.kind, 0 };
		if (tool.shapeDevice)
			wp_cursor_shape_device_v1_set_shape(tool.shapeDevice, serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR);
	}

	void WaylandSeat::Tablet::onProximityOut(Tool& tool) {
		tool.inside = false;
		tool.contactChange = -1;
		onFrame(tool, tool.sample.timeMs);
		mSeat.mWindow.onPointerSurfaceLeft();
	}

	void WaylandSeat::Tablet::onFrame(Tool& tool, uint32_t time) {
		tool.sample.timeMs = time;
		WindowInputObserver& input = mSeat.input();
		input.onPointerSample(tool.sample);

		if (tool.moved && tool.inside) mSeat.mWindow.onPointerSurfaceMove(tool.position);
		tool.moved = false;

		if (tool.contactChange != 0) {
			mSeat.mWindow.onPointerSurfaceButton(BTN_LEFT, tool.contactChange > 0, mSeat.mLastInputSerial);
			tool.contactChange = 0;
		}

		for (const auto& [button, pressed] : tool.buttons)
			mSeat.mWindow.onPointerSurfaceButton(
				button == InputID::MOUSE2 ? BTN_RIGHT : BTN_MIDDLE, pressed, mSeat.mLastInputSerial
			);
		tool.buttons.clear();
	}

	WaylandSeat::Clipboard::Clipboard(WaylandSeat& seat, wl_data_device_manager* manager, wl_seat* waylandSeat) :
		mSeat(seat),
		mManager(manager) {
		mDevice = wl_data_device_manager_get_data_device(manager, waylandSeat);
		wl_data_device_add_listener(mDevice, &kDataDeviceListener, this);
	}

	WaylandSeat::Clipboard::~Clipboard() {
		if (mSource) wl_data_source_destroy(mSource);
		if (mSelection) wl_data_offer_destroy(mSelection);
		if (mPendingOffer && mPendingOffer != mSelection) wl_data_offer_destroy(mPendingOffer);
		if (mDevice) wl_data_device_release(mDevice);
	}

	void WaylandSeat::Clipboard::setText(const std::string& text) {
		mOwnedText = text;
		if (mSource) wl_data_source_destroy(mSource);

		mSource = wl_data_device_manager_create_data_source(mManager);
		wl_data_source_add_listener(mSource, &kSourceListener, this);
		wl_data_source_offer(mSource, kTextMimeType);
		wl_data_source_offer(mSource, kLegacyTextMimeType);
		wl_data_device_set_selection(mDevice, mSource, mSeat.lastInputSerial());
		mSeat.mConnection.flush();
	}

	std::string WaylandSeat::Clipboard::text() {
		if (mSource) return mOwnedText;

		const bool hasText =
			std::find(mSelectionTypes.begin(), mSelectionTypes.end(), kTextMimeType) != mSelectionTypes.end();
		return hasText ? receive(kTextMimeType) : std::string();
	}

	ClipboardImage WaylandSeat::Clipboard::image() {
		if (mSource) return {};

		const std::string* chosen = nullptr;
		for (const std::string& type : mSelectionTypes) {
			if (type == kPngMimeType) {
				chosen = &type;
				break;
			}
			if (!chosen && type.rfind(kImageMimePrefix, 0) == 0) chosen = &type;
		}
		if (!chosen) return {};

		ClipboardImage image;
		image.mimeType = *chosen;
		image.bytes = receive(chosen->c_str());
		return image;
	}

	std::string WaylandSeat::Clipboard::receive(const char* mimeType) {
		if (!mSelection) return {};

		int pipeDescriptors[2];
		if (pipe2(pipeDescriptors, O_CLOEXEC) != 0) return {};

		wl_data_offer_receive(mSelection, mimeType, pipeDescriptors[1]);
		close(pipeDescriptors[1]);

		std::string bytes = readAll(pipeDescriptors[0], mSeat.mConnection.display());
		close(pipeDescriptors[0]);
		return bytes;
	}

	void WaylandSeat::Clipboard::onOffer(wl_data_offer* offer) {
		mPendingOffer = offer;
		mPendingOfferTypes.clear();
		wl_data_offer_add_listener(offer, &kOfferListener, this);
	}

	void WaylandSeat::Clipboard::onOfferMimeType(wl_data_offer* offer, const char* mimeType) {
		if (offer != mPendingOffer) return;
		mPendingOfferTypes.emplace_back(mimeType);
	}

	void WaylandSeat::Clipboard::onSelection(wl_data_offer* offer) {
		if (mSelection && mSelection != offer) wl_data_offer_destroy(mSelection);

		mSelection = offer;
		mSelectionTypes = offer && offer == mPendingOffer ? std::move(mPendingOfferTypes) : std::vector<std::string>{};
		mPendingOfferTypes.clear();
		mPendingOffer = nullptr;
	}

	void WaylandSeat::Clipboard::onSourceSend(const char*, int descriptor) {
		writeAll(descriptor, mOwnedText);
		close(descriptor);
	}

	void WaylandSeat::Clipboard::onSourceCancelled(wl_data_source* source) {
		if (source == mSource) mSource = nullptr;
		wl_data_source_destroy(source);
	}

	WaylandSeat::TextInput::TextInput(WaylandSeat& seat, zwp_text_input_manager_v3* manager, wl_seat* waylandSeat) :
		mSeat(seat) {
		mTextInput = zwp_text_input_manager_v3_get_text_input(manager, waylandSeat);
		zwp_text_input_v3_add_listener(mTextInput, &kTextInputListener, this);
	}

	WaylandSeat::TextInput::~TextInput() {
		if (mTextInput) zwp_text_input_v3_destroy(mTextInput);
	}

	void WaylandSeat::TextInput::setEnabled(bool enabled) {
		if (mEnabled == enabled) return;

		mEnabled = enabled;
		sendState();
	}

	void WaylandSeat::TextInput::setCursorRect(const RectF& surfaceRect) {
		if (mCursorRect == surfaceRect) return;

		mCursorRect = surfaceRect;
		if (mEnabled && mFocused) sendState();
	}

	void WaylandSeat::TextInput::onEnter() {
		mFocused = true;
		sendState();
	}

	void WaylandSeat::TextInput::onLeave() {
		mFocused = false;
		zwp_text_input_v3_disable(mTextInput);
		zwp_text_input_v3_commit(mTextInput);
	}

	void WaylandSeat::TextInput::onCommitString(const char* text) { mPendingCommit = text ? text : ""; }

	void WaylandSeat::TextInput::onDone() {
		if (!mPendingCommit.empty()) mSeat.input().onText(mPendingCommit);
		mPendingCommit.clear();
	}

	void WaylandSeat::TextInput::sendState() {
		if (!mFocused) return;

		if (mEnabled) {
			zwp_text_input_v3_enable(mTextInput);
			zwp_text_input_v3_set_content_type(
				mTextInput, ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE, ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL
			);
			zwp_text_input_v3_set_cursor_rectangle(
				mTextInput, (int32_t) mCursorRect.x, (int32_t) mCursorRect.y, (int32_t) mCursorRect.z, (int32_t) mCursorRect.w
			);
		} else {
			zwp_text_input_v3_disable(mTextInput);
		}

		zwp_text_input_v3_commit(mTextInput);
	}
}
