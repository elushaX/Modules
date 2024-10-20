#pragma once

#include "Widget.hpp"

#include <set>

namespace tp {
	class DebugTimeline {
		enum {
			STEP = 10,
			LEN = 200,
		};

	public:
		DebugTimeline() = default;

		void addSample(time_ms time) {
			samples[end] = time;
			start++;
			end++;
			shift();
		}

		[[nodiscard]] int size() const {
			return LEN - STEP;
		}

		[[nodiscard]] const time_ms* get() const {
			return samples + start;
		}

	private:
		void shift() {
			if (end >= LEN) {
				for (ualni i = STEP; i < LEN; i++) {
					samples[i - STEP] = samples[i];
				}
				start = 0;
				end = LEN - STEP;
			}
		}

	private:
		ualni start = 0;
		ualni end = LEN - STEP;

		time_ms samples[LEN + 1] {};
	};

	class DebugManager {
	public:
		DebugManager() = default;

		[[nodiscard]] bool isFrozen() const { return mDebugStopProcessing; }
		[[nodiscard]] bool isRedrawAlways() const { return mDebugRedrawAlways; }

		void update(RootWidget* rootWidget, EventHandler& events);
		void drawDebug(RootWidget* rootWidget, Canvas& canvas);

		void checkProcBreakPoints(Widget* widget) {
			if (mProcBreakpoints.find(widget) != mProcBreakpoints.end()) {
				mProcBreakpoints.erase(widget);
				DEBUG_BREAK(1)
			}
		}

		void checkLayoutBreakpoints(Widget* widget) {
			if (mLayBreakpoints.find(widget) != mLayBreakpoints.end()) {
				mLayBreakpoints.erase(widget);
				DEBUG_BREAK(1)
			}
		}

		[[nodiscard]] bool isDebug() const { return mDebug; }

	private:
		void recursiveDraw(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder);

		void drawPerformance();
		static void widgetMenu(Widget*);
		void drawLayoutOrder();

	private:
		RootWidget* mRootWidget = nullptr;

		// debug
		bool mDebug = false;
		bool mDebugStopProcessing = false;
		bool mDebugRedrawAlways = false;
		bool mDetailed = false;

		std::set<Widget*> mProcBreakpoints;
		std::set<Widget*> mLayBreakpoints;

	public:
		DebugTimeline mProcTime;
		DebugTimeline mUpdManager;
		DebugTimeline mLayManager;

		DebugTimeline mDrawTime;
	};

	extern DebugManager gDebugWidget;
}