#pragma once

struct android_app;

namespace tp {

	using AndroidMain = int (*)();

	void runAndroidApplication(
		android_app* app, AndroidMain main, const char* const* assetDirectories, int assetDirectoryCount
	);
}
