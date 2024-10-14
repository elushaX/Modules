#pragma once

#include "Widget.hpp"

#include <map>

namespace tp {
	class UpdateManager {
		friend DebugManager;

	public:
		UpdateManager() = default;

		void scheduleUpdate(Widget* widget, const char* reason);

		[[nodiscard]] bool isPendingUpdates() const {
			return !mTriggeredWidgets.empty();
		}

		void processWidgets(Widget* root, EventHandler& eventHandler);
		void clean();

		void updateTreeToProcess(Widget* root);
		void handleFocusChanges(Widget* root, EventHandler& events);

		void findFocusWidget(Widget* iter, Widget** focus, const Vec2F& pointer);
		static void getWidgetPath(Widget* widget, std::vector<Widget*>& out);
		void processActiveTree(Widget* iter, EventHandler& events, Vec2F pos);
		void processFocusItems(EventHandler& events);

	private:
		std::map<Widget*, bool> mTriggeredWidgets;
		Widget* mInFocusWidget = nullptr;

	private:
		int mDebugWidgetsToProcess = 0;
	};
}