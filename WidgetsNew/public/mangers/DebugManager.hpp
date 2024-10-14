#pragma once

#include "Widget.hpp"

namespace tp {
	class DebugManager {
	public:
		DebugManager() = default;

		bool isRedrawAlways() const { return mDebugRedrawAlways; }
		bool update(RootWidget* rootWidget, EventHandler& events);
		void drawDebug(RootWidget* rootWidget, Canvas& canvas);

	private:
		void recursiveDraw(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder);

		static void widgetMenu(Widget*);

	private:
		// debug
		bool mDebug = true;
		bool mDebugStopProcessing = false;
		bool mDebugRedrawAlways = false;
	};
}