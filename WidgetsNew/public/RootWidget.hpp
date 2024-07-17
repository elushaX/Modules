#pragma once

#include "Widget.hpp"

namespace tp {
	class RootWidget {

		// path from leaf to root of widgets that need to be updated
		struct ActivePath {
			struct ActiveNode {
				Widget* widget = nullptr;
				Vec2F globalPos;
			};

			Buffer<ActiveNode> path;
		};


	public:
		RootWidget() = default;

		void setRootWidget(Widget* widget);

		void processFrame(EventHandler* events, const RectF& screenArea);
		void drawFrame(Canvas& canvas);

		[[nodiscard]] bool needsUpdate() const;

		static void setWidgetArea(Widget& widget, const RectF& rect);

	private:
		void updateActivePaths(Buffer<ActivePath>* activePaths, const Vec2F& pointer);
		void drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos);

		void processPaths(const Buffer<ActivePath>& path, EventHandler& events);
		void clearProcessedFlag(const Buffer<ActivePath>& paths);

	private:
		Widget* mRoot = nullptr;

		Buffer<Widget*> mActiveWidgets;
		Buffer<ActivePath> mActivePaths[2]; // current and prev
		bool mFlipFlop = false;

		bool mNeedUpdate = false;
	};
}