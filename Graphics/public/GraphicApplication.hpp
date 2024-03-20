
#pragma once

#include "Window.hpp"
#include "Graphics.hpp"
#include "Timing.hpp"

namespace tp {
	class Application {
	public:
		Application();

		void run();

		virtual void processFrame(EventHandler* eventHandler);
		virtual void drawFrame(Canvas* canvas);

		virtual ~Application();

	protected:
		bool mInitialized = false;

		ualni mDrawPerSecond = 60;
		ualni mProcPerSecond = 160;

		Timer mDrawTimer;
		Timer mProcTimer;
		Timer mPerSecondTimer;

		halnf mFramesProcessedPerSecond = 0;
		halnf mFramesDrawnPerSecond = 0;

		halnf mFramesProcessed = 0;
		halnf mFramesDrawn = 0;

		Graphics* mGraphics = nullptr;
		Window* mWindow = nullptr;
	};
}