#pragma once

#include "Widget.hpp"

#include <map>
#include <vector>
#include <set>

namespace tp {
	class RootWidget : public WidgetManagerInterface {

		struct ActiveTreeNode {
			ActiveTreeNode* parent = nullptr;
			std::set<ActiveTreeNode*> children;
			Widget* widget = nullptr;
		};

	public:
		RootWidget() { setDebug("root", RGBA(1)); }

		void setRootWidget(Widget* widget);
		void updateWidget(Widget*);

		static void setWidgetArea(Widget& widget, const RectF& rect);

	public:
		void processFrame(EventHandler* events, const RectF& screenArea);
		void drawFrame(Canvas& canvas);

		[[nodiscard]] bool needsUpdate() const;

	private:
		void updateTreeToProcess();
		void updateAnimations(ActiveTreeNode* iter);
		void drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos);
		void drawDebug(Canvas& canvas, Widget* active, const Vec2F& pos);
		void findFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer);
		void handleFocusChanges(EventHandler& events);
		void getWidgetPath(Widget* widget, std::vector<Widget*>& out);
		void processActiveTree(ActiveTreeNode* iter, EventHandler& events);
		void processFocusItems(EventHandler& events);
		void adjustSizes(ActiveTreeNode* iter);

	private:
		RectF mScreenArea;
		Widget* mRoot = nullptr;

		// frame to frame changes
		std::set<Widget*> mTriggeredWidgets;
		std::map<Widget*, ActiveTreeNode> mWidgetsToProcess;
		Widget* mInFocusWidget = nullptr;

		bool mDebug = false;
	};
}