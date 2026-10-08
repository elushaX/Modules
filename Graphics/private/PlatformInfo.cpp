#include "PlatformInfo.hpp"

#include <cstdio>

namespace tp {

	const char* toString(OperatingSystem system) {
		switch (system) {
			case OperatingSystem::Linux: return "Linux";
			case OperatingSystem::Android: return "Android";
			case OperatingSystem::Windows: return "Windows";
			case OperatingSystem::MacOS: return "macOS";
			case OperatingSystem::IOS: return "iOS";
			case OperatingSystem::Unknown: break;
		}
		return "unknown";
	}

	const char* toString(FormFactor formFactor) {
		switch (formFactor) {
			case FormFactor::Desktop: return "desktop";
			case FormFactor::Tablet: return "tablet";
			case FormFactor::Phone: return "phone";
			case FormFactor::Unknown: break;
		}
		return "unknown";
	}

	std::string describe(const PlatformInfo& info) {
		char text[512];
		std::snprintf(
			text,
			sizeof(text),
			"%s %s, %s%s%s, scale %.2f, screen %.0fx%.0f, input:%s%s%s%s, window:%s%s%s",
			toString(info.system),
			info.systemVersion.c_str(),
			toString(info.formFactor),
			info.deviceName.empty() ? "" : " ",
			info.deviceName.c_str(),
			info.contentScale,
			info.screenSize.x,
			info.screenSize.y,
			info.hasMouse ? " mouse" : "",
			info.hasTouch ? " touch" : "",
			info.hasPen ? " pen" : "",
			info.hasPhysicalKeyboard ? " keyboard" : "",
			info.hasWindowControls ? " controls" : "",
			info.canMoveWindow ? " move" : "",
			info.canResizeWindow ? " resize" : ""
		);
		return text;
	}
}
