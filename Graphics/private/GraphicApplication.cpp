
#include "GraphicApplication.hpp"

using namespace tp;

Application::Application() {
	mWindow = Window::createWindow();
	mGraphics = new Graphics(mWindow);

	mDrawTimer.setDuration(1000.f / mDrawPerSecond);
	mProcTimer.setDuration(1000.f / mProcPerSecond);
	mPerSecondTimer.setDuration(1000.f);
}

void Application::run() {
	auto eventHandler = new EventHandler();

	mWindow->setEventHandler(eventHandler);

	mDrawTimer.reset();
	mProcTimer.reset();
	mPerSecondTimer.reset();

	bool redrawNeeded = false;

	// proc first frame by default
	{
		mWindow->processEvents();
		processFrame(eventHandler);

		mGraphics->drawBegin();
		drawFrame(mGraphics->getCanvas());
		mGraphics->drawEnd();
		mWindow->draw();
	}

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
				mGraphics->drawBegin();

				drawFrame(mGraphics->getCanvas());

				mGraphics->drawEnd();

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
	delete mGraphics;
	Window::destroyWindow(mWindow);
}