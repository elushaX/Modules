#pragma once

#include "PlatformInfo.hpp"

#include <string>

struct android_app;
struct AInputEvent;

namespace tp::android {

	[[nodiscard]] android_app* application();
	void setApplication(android_app* app);

	[[nodiscard]] std::string unicodeText(android_app& app, const AInputEvent* event);
	void setSoftKeyboardVisible(android_app& app, bool visible);
	[[nodiscard]] EdgeInsets systemInsets(android_app& app);
	bool openUrl(android_app& app, const std::string& url);

	void extractAssets(android_app& app, const char* directory);
}
