#include "AndroidEntry.hpp"

#include "AndroidPlatform.hpp"

#include <android/log.h>
#include <android_native_app_glue.h>

#include <cstdlib>
#include <exception>
#include <unistd.h>

namespace tp {

	void runAndroidApplication(
		android_app* app, AndroidMain main, const char* const* assetDirectories, int assetDirectoryCount
	) {
		android::setApplication(app);
		for (int index = 0; index < assetDirectoryCount; index++)
			android::extractAssets(*app, assetDirectories[index]);
		setenv("HOME", app->activity->internalDataPath, 1);
		if (chdir(app->activity->internalDataPath) != 0)
			__android_log_print(ANDROID_LOG_ERROR, "modules", "cannot enter %s", app->activity->internalDataPath);

		try {
			main();
		} catch (const std::exception& error) {
			__android_log_print(ANDROID_LOG_FATAL, "modules", "application failed: %s", error.what());
		}

		android::setApplication(nullptr);
		while (!app->destroyRequested) {
			int events = 0;
			android_poll_source* source = nullptr;
			if (ALooper_pollOnce(-1, nullptr, &events, (void**) &source) >= 0 && source) source->process(app, source);
		}
	}
}
