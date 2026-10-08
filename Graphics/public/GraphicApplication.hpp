
#pragma once

#include "Window.hpp"
#include "Graphics.hpp"
#include "Timing.hpp"
#include "HangWatchdog.hpp"

#include <memory>
#include <vector>

namespace tp {
	class Application {
	public:
		explicit Application(const WindowIdentity& identity = WindowIdentity::generic());

		void run();

		void start();
		void step(halnf deltaMs);
		void finish();
		[[nodiscard]] bool isRunning() const { return mEventHandler != nullptr; }
		[[nodiscard]] Window& window() { return *mWindow; }

		virtual bool forceNewFrame() { return false; }

		virtual void processFrame(EventHandler* eventHandler, halnf deltaTime);
		virtual void drawFrame(Canvas* canvas);

		virtual ~Application();

		void drawDebug();

		void addHangContributor(HangWatchdog::Contributor contributor);
		[[nodiscard]] HangWatchdog* hangWatchdog() { return mHangWatchdog.get(); }

	protected:
		[[nodiscard]] virtual HangSettings hangSettings() const;

	private:
		void enterPhase(UiPhase phase);
		void runFrame();
		[[nodiscard]] bool wantsFrame();
		void runFrame(halnf deltaMs);
		void processPendingInput(halnf deltaMs);
		void present();
		void updateFrameCounters();

		EventHandler* mEventHandler = nullptr;
		time_ms mPreviousFrameTime = 0;
		time_ns mFrameStart = 0;
		std::unique_ptr<HangWatchdog> mHangWatchdog;
		std::vector<HangWatchdog::Contributor> mHangContributors;

	protected:
		bool mInitialized = false;

		Timer mPerSecondTimer;

		halnf mFramesProcessedPerSecond = 0;
		halnf mFramesDrawnPerSecond = 0;

		halnf mFramesProcessed = 0;
		halnf mFramesDrawn = 0;

		Graphics* mGraphics = nullptr;
		Window* mWindow = nullptr;
	};
}