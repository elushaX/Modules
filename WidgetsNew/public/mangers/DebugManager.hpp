#pragma once

#include "Widget.hpp"

#include <set>

namespace tp {
	class DebugManager {
	public:
		DebugManager() = default;

		bool isRedrawAlways() const { return mDebugRedrawAlways; }
		bool update(RootWidget* rootWidget, EventHandler& events);
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

	private:
		void recursiveDraw(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder);

		static void widgetMenu(Widget*);
		void drawLayoutOrder();

	private:
		RootWidget* mRootWidget = nullptr;

		// debug
		bool mDebug = false;
		bool mDebugStopProcessing = false;
		bool mDebugRedrawAlways = false;
		bool mSlowMotion = false;

		std::set<Widget*> mProcBreakpoints;
		std::set<Widget*> mLayBreakpoints;
	};

	extern DebugManager gDebugWidget;
}