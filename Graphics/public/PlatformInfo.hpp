#pragma once

#include "Vec.hpp"

#include <string>

namespace tp {

	enum class OperatingSystem { Linux, Android, Windows, MacOS, IOS, Unknown };

	enum class FormFactor { Desktop, Tablet, Phone, Unknown };

	struct EdgeInsets {
		halnf left = 0;
		halnf top = 0;
		halnf right = 0;
		halnf bottom = 0;

		[[nodiscard]] bool operator==(const EdgeInsets& other) const = default;
	};

	struct PlatformInfo {
		OperatingSystem system = OperatingSystem::Unknown;
		FormFactor formFactor = FormFactor::Unknown;
		std::string systemVersion;
		std::string deviceName;

		halnf contentScale = 1;
		Vec2F screenSize = { 0, 0 };
		EdgeInsets safeInsets;

		bool hasMouse = false;
		bool hasTouch = false;
		bool hasPen = false;
		bool hasPhysicalKeyboard = false;

		bool hasWindowControls = false;
		bool canMoveWindow = false;
		bool canResizeWindow = false;
		bool hasHoverPointer = false;

		[[nodiscard]] bool isTouchFirst() const {
			return formFactor == FormFactor::Tablet || formFactor == FormFactor::Phone;
		}
	};

	[[nodiscard]] const char* toString(OperatingSystem system);
	[[nodiscard]] const char* toString(FormFactor formFactor);
	[[nodiscard]] std::string describe(const PlatformInfo& info);
}
