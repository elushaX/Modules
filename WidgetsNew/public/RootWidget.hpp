#pragma once

#include "Widget.hpp"

#include <map>
#include <vector>
#include <set>

namespace tp {
	class RootWidget : public WidgetManagerInterface {
	public:
		RootWidget() { setDebug("root", RGBA(1)); }

		void setRootWidget(Widget* widget);
		void updateWidget(Widget*, const char* reason = nullptr) override;

		static void setWidgetArea(Widget& widget, const RectF& rect);

	public:
		void processFrame(EventHandler* events, const RectF& screenArea);
		void drawFrame(Canvas& canvas);

		[[nodiscard]] bool needsUpdate() const;

	private:
		void updateTreeToProcess();
		void updateAnimations();
		void processLayout();
		void drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos);
		void drawDebug(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder);
		void findFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer);
		void handleFocusChanges(EventHandler& events);
		void getWidgetPath(Widget* widget, std::vector<Widget*>& out);
		void processActiveTree(Widget* iter, EventHandler& events, Vec2F pos);
		void processFocusItems(EventHandler& events);

		void updateAreaCache(Widget* iter, bool read);
		static void debugDrawWidget(Widget* widget);

	private:
		LayoutManager mLayoutManager;

		RectF mScreenArea;
		Widget* mRoot = nullptr;

		// frame to frame changes
		std::map<Widget*, bool> mTriggeredWidgets;
		Widget* mInFocusWidget = nullptr;

		bool mDebug = true;
		bool mDebugStopProcessing = false;
		bool mDebugRedrawAlways = false;
		int mDebugWidgetsToProcess = 0;
	};
}