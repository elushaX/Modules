
#include "Window.hpp"
#include "Graphics.hpp"

#include "Timing.hpp"

#include "imgui.h"

using namespace tp;

class Application {
public:
	Application() :
		mApplicaitonModule("Application", nullptr, nullptr, mModuleDeps) {
		mInitialized = mApplicaitonModule.initialize();
		if (!mInitialized) return;

		mWindow = Window::createWindow();

		mDrawTimer.setDuration(1000.f / mDrawPerSecond);
		mProcTimer.setDuration(1000.f / mProcPerSecond);
		mPerSecondTimer.setDuration(1000.f);
	}

	void run() {
		Graphics graphics(mWindow);
		auto eventHandler = new EventHandler();

		mWindow->setEventHandler(eventHandler);

		mDrawTimer.reset();
		mProcTimer.reset();
		mPerSecondTimer.reset();

		bool redrawNeeded = false;

		while (!mWindow->shouldClose()) {
			if (mProcTimer.isTimeout()) {

				mWindow->processEvents();

				while (eventHandler->isEvents()) {
					eventHandler->processEvent();
					processFrame(eventHandler);

					redrawNeeded = true;
					mFramesProcessed++;
				}

				// graphics.procBegin();
				// graphics.procEnd();
				redrawNeeded = true;
				mFramesProcessed++;

				mProcTimer.wait();
				mProcTimer.reset();
			}

			if (mDrawTimer.isTimeout()) {
				mDrawTimer.reset();

				if (redrawNeeded) {
					graphics.drawBegin();

					drawFrame(graphics.getCanvas());

					graphics.drawEnd();

					mWindow->draw();

					redrawNeeded = false;
					mFramesDrawn++;
				}
			}

			if (mPerSecondTimer.isTimeout()) {
				mPerSecondTimer.reset();
				
				mFramesProcessedPerSecond = mFramesProcessed;
				mFramesDrawnPerSecond = mFramesDrawn;

				mFramesDrawn = 0;
				mFramesProcessed = 0;
			}
		}

		delete eventHandler;
	}

	virtual void processFrame(EventHandler* eventHandler) { 
	}

	virtual void drawFrame(Canvas* canvas) {
		ImGui::Text("Frames processed per second: %f", mFramesProcessedPerSecond);
		ImGui::Text("Frames drawn per second: %f", mFramesDrawnPerSecond);
	}

	~Application() { 
		if (!mInitialized) return;
		Window::destroyWindow(mWindow);
		mApplicaitonModule.deinitialize();
	}

private:
	ModuleManifest* mModuleDeps[2] = { &gModuleGraphics, nullptr };
	ModuleManifest mApplicaitonModule;

	Window* mWindow = nullptr;

	bool mInitialized = false;

	ualni mDrawPerSecond = 60;
	ualni mProcPerSecond = 1000;

	Timer mDrawTimer;
	Timer mProcTimer;
	Timer mPerSecondTimer;

	halnf mFramesProcessedPerSecond = 0;
	halnf mFramesDrawnPerSecond = 0;

	halnf mFramesProcessed = 0;
	halnf mFramesDrawn = 0;
};

int main() { 
	Application app;
	app.run();
}
