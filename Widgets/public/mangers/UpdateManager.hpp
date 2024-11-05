#pragma once

#include "Widget.hpp"

#include <map>

namespace tp {
	// FIXME : desperately needs refactor
	class UpdateManager {
		friend DebugManager;

	public:
		UpdateManager() = default;

		void scheduleUpdate(Widget* widget, const char* reason);

		void lockFocus(Widget* widget);
		void freeFocus(Widget* widget);

		[[nodiscard]] bool isPendingUpdates() const {
			return !mTriggeredWidgets.empty();
		}

		void processWidgets(Widget* root, EventHandler& eventHandler);
		void clean();

		void updateTreeToProcess(Widget* root);
		void handleFocusChanges(Widget* active, Widget* prevActive);
		Widget* findFocusWidget(Widget* root, EventHandler& events);

		void findMouseFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer);
		static void getWidgetPath(Widget* widget, std::vector<Widget*>& out);
		void processActiveTree(Widget* iter, EventHandler& events, Vec2F pos);
		void processFocusItems(EventHandler& events);

		Widget* getFocusWidget() { return mInFocusWidget; }

	private:
		static void procWidget(Widget* widget, EventHandler& events, bool withEvents = false);

	private:
		std::map<Widget*, bool> mTriggeredWidgets;
		Widget* mInFocusWidget = nullptr;
		Widget* mFocusLockWidget = nullptr;

	private:
		int mDebugWidgetsToProcess = 0;
	};
}