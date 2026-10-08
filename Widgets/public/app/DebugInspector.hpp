#pragma once

#include "Diagnostics.hpp"
#include "Sizing.hpp"
#include "EventHandler.hpp"
#include "InputTranslator.hpp"
#include "Painter.hpp"
#include "Ui.hpp"

#include <deque>
#include <set>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	class TimeSeries {
	public:
		static constexpr int kCapacity = 240;

		void add(float value);
		[[nodiscard]] const float* data() const { return mValues; }
		[[nodiscard]] int count() const { return kCapacity; }
		[[nodiscard]] int offset() const { return mNext; }
		[[nodiscard]] float latest() const;
		[[nodiscard]] float average() const;
		[[nodiscard]] float maximum() const;

	private:
		float mValues[kCapacity] = {};
		int mNext = 0;
	};

	class DebugInspector : public UiObserver {
	public:
		explicit DebugInspector(Ui& ui);
		~DebugInspector() override;

		DebugInspector(const DebugInspector&) = delete;
		DebugInspector& operator=(const DebugInspector&) = delete;

		[[nodiscard]] static AxisSizing
		editedSizing(AxisSizing::Mode mode, halnf length, halnf weight, halnf minimum, halnf maximum);

		void handleShortcuts(const EventHandler& input);
		void setToggleKey(std::optional<InputID> key) { mToggleKey = key; }
		[[nodiscard]] std::optional<InputID> toggleKey() const { return mToggleKey; }

		struct WindowSplit {
			RectF content;
			RectF panel;
		};

		[[nodiscard]] WindowSplit splitWindow(Vec2F windowSize) const;
		[[nodiscard]] InputRouting inputRouting(Vec2F pointer, Vec2F windowSize) const;

		void setVisible(bool visible) { mVisible = visible; }
		[[nodiscard]] bool isVisible() const { return mVisible; }
		[[nodiscard]] bool isProcessingFrozen() const { return mVisible && mFreezeProcessing; }
		[[nodiscard]] bool forcesRedraw() const { return mVisible && mForceRedraw; }

		void recordFrame(halnf processMs, halnf drawMs, halnf framesProcessedPerSecond, halnf framesDrawnPerSecond);

		struct WindowCounters {
			uint64_t inputReceived = 0;
			uint64_t inputDelivered = 0;
			uint64_t configuresReceived = 0;
			uint64_t configuresApplied = 0;
			uint64_t swapchainRecreations = 0;
		};
		void recordWindowCounters(const WindowCounters& counters) { mWindowCounters = counters; }
		[[nodiscard]] const WindowCounters& windowCounters() const { return mWindowCounters; }
		void draw();
		void paintOverlay(Painter& painter);

	private:
		std::optional<InputID> mToggleKey;
		WindowCounters mWindowCounters;
		struct EventRecord {
			ualni frame;
			std::string type;
			std::string receiver;
			Vec2F position;
			bool accepted;
		};

		void onEventDelivering(const Widget& receiver, const Event& event) override;
		void onEventDelivered(const Widget& receiver, const Event& event) override;
		void onLayoutPassStarted() override;
		void onWidgetArranged(const Widget& widget) override;
		void onWidgetDetached(const Widget& widget) override;

		void drawPanelWidthControl();
		void drawToolbar();
		void drawPerformanceTab();
		void drawHoverStackTab();
		void drawWidgetTreeTab();
		void drawWidgetTreeNode(Widget& widget);
		void drawSelectedWidget();
		void drawEventsTab();
		void drawLayoutTab();
		void drawPopupsTab();

		void drawSizingEditor(Widget& widget, Axis axis);
		void drawWidgetLink(Widget& widget);
		void paintOutlines(Painter& painter, const Widget& widget, int depth);

		[[nodiscard]] static std::string nameOf(const Widget& widget);
		[[nodiscard]] static const char* eventTypeName(const Event& event);

		Ui& mUi;

		bool mVisible = false;
		float mPanelFraction = 0.32f;
		bool mFreezeProcessing = false;
		bool mForceRedraw = false;
		bool mShowOutlines = false;
		bool mLogPointerMoves = false;

		Widget* mSelected = nullptr;
		std::set<const Widget*> mEventBreakpoints;
		std::set<const Widget*> mLayoutBreakpoints;

		ualni mFrame = 0;
		std::deque<EventRecord> mEvents;
		std::vector<std::string> mLastLayoutOrder;
		std::vector<std::string> mCurrentLayoutOrder;

		FrameStats mPreviousStats;
		TimeSeries mProcessMs;
		TimeSeries mDrawMs;
		TimeSeries mInputMs;
		TimeSeries mAnimationMs;
		TimeSeries mLayoutMs;
		TimeSeries mPaintMs;
		TimeSeries mArranged;
		TimeSeries mMeasured;
		halnf mFramesProcessedPerSecond = 0;
		halnf mFramesDrawnPerSecond = 0;
	};
}
