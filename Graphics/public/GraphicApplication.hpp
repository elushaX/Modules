
#pragma once

#include "Window.hpp"
#include "Graphics.hpp"
#include "Timing.hpp"

namespace tp {
	class Application {
	public:
		Application();

		void run();

		virtual bool forceNewFrame() { return false; }

		virtual void processFrame(EventHandler* eventHandler, halnf deltaTime);
		virtual void drawFrame(Canvas* canvas);

		virtual ~Application();

		void drawDebug();

	private:
		void runDefaultLoop();
		void runDebugLoop();

	protected:
		bool mInitialized = false;

		ualni mDrawPerSecond = 1160;
		ualni mProcPerSecond = 1300;

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