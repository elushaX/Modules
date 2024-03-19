
#include "GraphicApplication.hpp"

using namespace tp;

Application::Application() {
	mWindow = Window::createWindow();

	mDrawTimer.setDuration(1000.f / mDrawPerSecond);
	mProcTimer.setDuration(1000.f / mProcPerSecond);
	mPerSecondTimer.setDuration(1000.f);
}

void Application::run() {
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

void Application::processFrame(EventHandler* eventHandler) {}

void Application::drawFrame(Canvas* canvas) {
	// ImGui::Text("Frames processed per second: %f", mFramesProcessedPerSecond);
	// ImGui::Text("Frames drawn per second: %f", mFramesDrawnPerSecond);
}

Application::~Application() {
	Window::destroyWindow(mWindow);
}