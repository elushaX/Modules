#pragma once

#include "Diagnostics.hpp"
#include "Event.hpp"
#include "Popup.hpp"
#include "Theme.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	class Widget;
	class Painter;
	class TextMetrics;
	class ImageLoader;
	class LayoutEngine;
	class InputRouter;
	class PopupStack;
	class TickScheduler;
	class TooltipController;
	class WindowHost;

	enum class InteractionMode { Pointer, Touch };

	class Ui {
	public:
		static constexpr halnf kLongPressMs = 500;

		explicit Ui(const Theme& theme = Theme::standard());
		~Ui();

		Ui(const Ui&) = delete;
		Ui& operator=(const Ui&) = delete;

		void setRoot(Widget* root);
		[[nodiscard]] Widget* root() const { return mRoot; }

		void setViewport(const RectF& viewport);
		void resizeViewport(const RectF& viewport);
		[[nodiscard]] const RectF& viewport() const { return mViewport; }

		void setWindowHost(WindowHost* host);
		[[nodiscard]] WindowHost* windowHost() const { return mWindowHost; }
		void refreshCursor();

		void setTheme(const Theme& theme);
		void setInteractionMode(InteractionMode mode);
		[[nodiscard]] InteractionMode interactionMode() const { return mInteractionMode; }
		[[nodiscard]] bool isTouchMode() const { return mInteractionMode == InteractionMode::Touch; }
		[[nodiscard]] const Theme& theme() const { return mTheme; }

		void setImageLoader(ImageLoader* loader) { mImageLoader = loader; }
		[[nodiscard]] ImageLoader* imageLoader() const { return mImageLoader; }

		void setTextMetrics(const TextMetrics* metrics);
		[[nodiscard]] const TextMetrics& textMetrics() const;
		[[nodiscard]] static const TextMetrics& fallbackTextMetrics();

		void dispatch(Event event);
		void setOnEventDispatched(std::function<void()> callback) { mOnEventDispatched = std::move(callback); }
		void setOnAdvance(std::function<void(halnf)> callback) { mOnAdvance = std::move(callback); }
		void advance(halnf deltaMs);
		void updateLayout();
		void paint(Painter& painter);
		[[nodiscard]] bool needsFrame() const;

		void openPopup(const PopupRequest& request);
		void closePopup(const Widget& content);
		void closePopupChain(const Widget& content);
		void closeAllPopups();
		[[nodiscard]] bool isPopupOpen(const Widget& content) const;
		[[nodiscard]] const RectF* popupAnchorOf(const Widget& content) const;
		[[nodiscard]] ualni openPopupCount() const;

		void capturePointer(Widget& widget);
		void releasePointer(const Widget& widget);
		[[nodiscard]] Widget* pointerCapture() const;

		void setKeyboardFocus(Widget* widget);
		[[nodiscard]] Widget* keyboardFocus() const;
		[[nodiscard]] bool isKeyboardNavigating() const;
		void setActiveWidget(Widget* widget);
		[[nodiscard]] Widget* activeWidget() const;

		[[nodiscard]] Widget* hoveredWidget() const;
		[[nodiscard]] Vec2F pointerPosition() const;
		[[nodiscard]] bool isPrimaryButtonDown() const;
		[[nodiscard]] bool isKeyDown(InputID key) const;
		[[nodiscard]] Widget* widgetAt(Vec2F windowPoint) const;

		[[nodiscard]] Widget* openTooltip() const;
		[[nodiscard]] std::vector<Widget*> openPopups() const;
		[[nodiscard]] std::vector<Widget*> animatingWidgets() const;
		[[nodiscard]] bool isLayoutPending() const { return mLayoutPending; }
		[[nodiscard]] bool isPaintPending() const { return mPaintPending; }

		[[nodiscard]] const FrameStats& stats() const { return mDiagnostics.stats; }
		void resetStats() { mDiagnostics.stats = {}; }
		void setObserver(UiObserver* observer) { mDiagnostics.observer = observer; }
		[[nodiscard]] std::string dumpTree() const;

	private:
		friend class Widget;
		friend class WidgetInternals;

		static constexpr halnf kMaximumFrameDeltaMs = 50;

		void scheduleLayout() { mLayoutPending = true; }
		[[nodiscard]] bool isViewportResizing() const { return mViewportResizing; }
		void schedulePaint() { mPaintPending = true; }
		void startTicking(Widget& widget);
		void stopTicking(const Widget& widget);
		void startGeometryAnimation(Widget& widget);
		void onPopupClosed(const PopupRequest& request);
		void onWidgetHidden(Widget& widget);
		void onWidgetDetached(Widget& widget);

		void ensureLayout();
		void layoutPopups();
		[[nodiscard]] RectF popupOrigin(const PopupRequest& request) const;
		void paintSubtree(Painter& painter, Widget& widget, const RectF& visibleArea);
		void paintChildren(Painter& painter, Widget& widget, const RectF& visibleArea);

		Theme mTheme;
		InteractionMode mInteractionMode = InteractionMode::Pointer;
		RectF mViewport = makeRect(0, 0, 0, 0);
		Widget* mRoot = nullptr;
		const TextMetrics* mTextMetrics = nullptr;
		ImageLoader* mImageLoader = nullptr;
		WindowHost* mWindowHost = nullptr;

		std::unique_ptr<LayoutEngine> mLayoutEngine;
		std::unique_ptr<PopupStack> mPopups;
		std::unique_ptr<InputRouter> mInput;
		std::unique_ptr<TickScheduler> mTicks;
		std::unique_ptr<TooltipController> mTooltips;

		std::function<void()> mOnEventDispatched;
		std::function<void(halnf)> mOnAdvance;
		bool mLayoutPending = true;
		bool mViewportResizing = false;
		bool mPaintPending = true;
		Diagnostics mDiagnostics;
	};
}
