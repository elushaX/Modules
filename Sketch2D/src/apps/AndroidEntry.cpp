#include "AndroidEntry.hpp"
#include "AndroidUpdater.hpp"
#include "Sketch2DApplication.hpp"
#include "ThreadPlacement.hpp"

namespace {
	android_app* gApp = nullptr;

	int runSketch2D() {
		tp::ThreadPlacement::reserveCallingThreadForInteraction();
		tp::sketch2d::AndroidUpdateBackend updates(*gApp);
		tp::sketch2d::Sketch2DApplication application;
		application.checkForUpdates(updates);
		application.run();
		return 0;
	}
}

extern "C" void android_main(android_app* app) {
	gApp = app;
	const char* assets[] = { "rsc", "rsc/icons" };
	tp::runAndroidApplication(app, runSketch2D, assets, 2);
}
