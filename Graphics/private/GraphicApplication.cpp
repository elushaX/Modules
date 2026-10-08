#include <cstdio>

#include "GraphicApplication.hpp"
#include "Profiler.hpp"

#include "imgui.h"

using namespace tp;

Application::Application(const WindowIdentity& identity) {
	mWindow = Window::createWindow({ 1500, 900 }, identity);
	mGraphics = new Graphics(mWindow);
	mPerSecondTimer.setDuration(1000.f);
}

void Application::start() {
	mHangWatchdog = std::make_unique<HangWatchdog>(hangSettings());
	for (const HangWatchdog::Contributor& contributor : mHangContributors)
		mHangWatchdog->addContributor(contributor);
	mEventHandler = new EventHandler();
	mWindow->setEventHandler(mEventHandler);

	mWindow->waitForReady();

	mPreviousFrameTime = get_time();
	mPerSecondTimer.reset();
	runFrame();
}

void Application::step(halnf deltaMs) {
	enterPhase(UiPhase::Idle);
	mWindow->processEvents(false);
	runFrame(deltaMs);
}

void Application::finish() {
	mHangWatchdog.reset();
	mWindow->setEventHandler(nullptr);
	delete mEventHandler;
	mEventHandler = nullptr;
}

void Application::run() {
	start();

	while (!mWindow->shouldClose()) {
		enterPhase(UiPhase::Idle);
		mWindow->processEvents(!wantsFrame() || mWindow->isWaitingForFrame());
		while (mWindow->isWaitingForFrame() && !mWindow->shouldClose())
			mWindow->processEvents(true);
		runFrame();
	}

	finish();
}

void Application::addHangContributor(HangWatchdog::Contributor contributor) {
	if (mHangWatchdog) mHangWatchdog->addContributor(contributor);
	mHangContributors.push_back(std::move(contributor));
}

HangSettings Application::hangSettings() const {
	HangSettings settings;
	settings.application = HangWatchdog::processName();
	return settings;
}

void Application::enterPhase(UiPhase phase) {
	if (mHangWatchdog) mHangWatchdog->enterPhase(phase);
}

bool Application::wantsFrame() { return forceNewFrame() || mWindow->hasPendingWork(); }

void Application::runFrame() {
	updateGlobalTime();

	const time_ms now = get_time();
	const halnf deltaMs = halnf(now - mPreviousFrameTime);
	mPreviousFrameTime = now;
	runFrame(deltaMs);
}

void Application::runFrame(halnf deltaMs) {
	mFrameStart = get_time_ns();
	Profiler::global().beginFrame();

	{
		enterPhase(UiPhase::Input);
		TP_PROFILE_SCOPE("frame.process");
		processPendingInput(deltaMs);
	}
	enterPhase(UiPhase::Draw);
	present();
	updateFrameCounters();

	Profiler::global().endFrame();
}

void Application::processPendingInput(halnf deltaMs) {
	halnf deltaTime = deltaMs;
	mWindow->flushPendingInput();

	do {
		TP_PROFILE_COUNT("input.events", 1);
		if (mHangWatchdog) mHangWatchdog->noteInputEvent();
		mEventHandler->processEvent();
		processFrame(mEventHandler, deltaTime);
		deltaTime = 0;
		mFramesProcessed++;
	} while (mEventHandler->hasQueuedEvents());
}

void Application::present() {
	TP_PROFILE_SCOPE("frame.present");
	mGraphics->drawBegin();
	{
		TP_PROFILE_SCOPE("frame.draw");
		drawFrame(mGraphics->getCanvas());
	}
	enterPhase(UiPhase::Present);
	mGraphics->drawEnd();
	mWindow->draw();
	mFramesDrawn++;
}

void Application::updateFrameCounters() {
	if (!mPerSecondTimer.isTimeout()) return;
	mPerSecondTimer.reset();

	mFramesProcessedPerSecond = mFramesProcessed;
	mFramesDrawnPerSecond = mFramesDrawn;
	mFramesDrawn = 0;
	mFramesProcessed = 0;
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