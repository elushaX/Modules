
#include "GraphicApplication.hpp"

#include "imgui.h"

using namespace tp;

Application::Application() {
	mWindow = Window::createWindow();
	mGraphics = new Graphics(mWindow);

	mDrawTimer.setDuration(1000.f / mDrawPerSecond);
	mProcTimer.setDuration(1000.f / mProcPerSecond);
	mPerSecondTimer.setDuration(1000.f);
}

void Application::runDefaultLoop() {
	auto eventHandler = new EventHandler();

	mWindow->setEventHandler(eventHandler);

	mDrawTimer.reset();
	mProcTimer.reset();
	mPerSecondTimer.reset();

	bool redrawNeeded = false;

	// proc first frame by default
	{
		mWindow->processEvents(false);
		processFrame(eventHandler, 0);

		mGraphics->drawBegin();
		drawFrame(mGraphics->getCanvas());
		mGraphics->drawEnd();
		mWindow->draw();
	}

	time_ms prevProcTime = get_time();

	bool isForcedNewFrame = true;

	while (!mWindow->shouldClose()) {
		mWindow->processEvents(!isForcedNewFrame);
		updateGlobalTime();

		if (mProcTimer.isTimeout() || eventHandler->isEvents() || isForcedNewFrame) {

			while ((eventHandler->isEvents() || isForcedNewFrame)) {

				eventHandler->processEvent();

				time_ms currentTime = get_time();
				processFrame(eventHandler, halnf(currentTime - prevProcTime));
				prevProcTime = currentTime;

				redrawNeeded = true;
				mFramesProcessed++;

				isForcedNewFrame = forceNewFrame();

				if (isForcedNewFrame) {
					// mWindow->processEvents();
					break;
				}
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

void Application::runDebugLoop() {
	auto eventHandler = new EventHandler();
	mWindow->setEventHandler(eventHandler);

	time_ms prevProcTime = get_time();

	while (!mWindow->shouldClose()) {
		mWindow->processEvents();
		eventHandler->processAllEvent();
		time_ms currentTime = get_time();
		processFrame(eventHandler, halnf(currentTime - prevProcTime));
		prevProcTime = currentTime;


		mGraphics->drawBegin();
		drawFrame(mGraphics->getCanvas());
		mGraphics->drawEnd();
		mWindow->draw();
	}

	delete eventHandler;
}

void Application::run() {
	runDefaultLoop();
	// runDebugLoop();
}

void Application::processFrame(EventHandler* eventHandler, halnf deltaTime) {}

void Application::drawFrame(Canvas* canvas) {
	// ImGui::Text("Frames processed per second: %f", mFramesProcessedPerSecond);
	// ImGui::Text("Frames drawn per second: %f", mFramesDrawnPerSecond);
}

void Application::drawDebug() {
	ImGui::Text("Frames processed per second: %f", this->mFramesProcessedPerSecond);
	ImGui::Text("Frames drawn per second: %f", this->mFramesDrawnPerSecond);
}

Application::~Application() {
	delete mGraphics;
	Window::destroyWindow(mWindow);
}