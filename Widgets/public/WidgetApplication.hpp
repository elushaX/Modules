#pragma once

#include "GraphicApplication.hpp"
#include "RootWidget.hpp"
#include "Timing.hpp"

namespace tp {
	class WidgetApplication : public Application {
	public:
		WidgetApplication() = default;

		void setRoot(Widget* widget);
		virtual void debugUI();

	private:
		void processFrame(EventHandler* eventHandler, halnf deltaTime) override;

		void drawFrame(Canvas* canvas) override;
		bool forceNewFrame() override ;


	private:
		halnf mDebugSplitFactor = 0.7;

		RootWidget mRootWidget;

	private:
		RectF mGuiArea;
		RectF mDebugArea;
	};
}