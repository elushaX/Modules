#pragma once

#include "GraphicApplication.hpp"
#include "RootWidget.hpp"
#include "Timing.hpp"

namespace tp {
	class WidgetApplication : public Application {
	public:
		WidgetApplication() = default;

		void setRoot(Widget* widget) {
			mRootWidget.setRootWidget(widget);
		}

	private:
		void processFrame(EventHandler* eventHandler, halnf deltaTime) override {
			const auto rec = RectF({ 0, 0 }, mWindow->getSize());

			Timer timer;
			mRootWidget.processFrame(eventHandler, rec);
			mProcTime = timer.timePassed();
		}

		void drawFrame(Canvas* canvas) override {
			Timer timer;
			mRootWidget.drawFrame(*canvas);
			mDrawTime = timer.timePassed();

			if (mRootWidget.isDebug()) {
				drawDebug();
				debugUI();
			}
		}

		bool forceNewFrame() override {
			return mRootWidget.needsUpdate();
		}

	private:
		void debugUI();

	private:
		time_ms mProcTime = 0;
		time_ms mDrawTime = 0;

		RootWidget mRootWidget;
	};
}