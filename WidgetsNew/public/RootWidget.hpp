#pragma once

#include "Widget.hpp"

#include "UpdateManager.hpp"
#include "LayoutManager.hpp"
#include "DebugManager.hpp"

namespace tp {
	class RootWidget : public WidgetManagerInterface {
		friend DebugManager;

	public:
		RootWidget();

		// User Interface
	public:
		void setRootWidget(Widget* widget);
		static void setWidgetArea(Widget& widget, const RectF& rect);

		// Graphic Application Interface
	public:
		void processFrame(EventHandler* events, const RectF& screenArea);
		void drawFrame(Canvas& canvas);
		[[nodiscard]] bool needsUpdate() const;

		// Internals
	private:
		void drawRecursion(Canvas& canvas, Widget* active, const Vec2F& pos);

		void updateWidget(Widget*, const char* reason = nullptr) override;
		void updateAnimations();
		void updateAreaCache(Widget* iter, bool read);

	private:
		Widget* mRoot = nullptr;

		LayoutManager mLayoutManager;
		UpdateManager mUpdateManager;
	};
}