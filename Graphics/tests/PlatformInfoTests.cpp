#include "UnitTest++/UnitTest++.h"

#include "PlatformInfo.hpp"

using namespace tp;

SUITE(PlatformInfo) {
	TEST(DefaultsDescribeAnUnknownPlatformWithoutWindowChrome) {
		const PlatformInfo info;

		CHECK(info.system == OperatingSystem::Unknown);
		CHECK(info.formFactor == FormFactor::Unknown);
		CHECK(!info.hasWindowControls);
		CHECK(!info.isTouchFirst());
	}

	TEST(TabletsAndPhonesAreTouchFirst) {
		PlatformInfo info;

		info.formFactor = FormFactor::Tablet;
		CHECK(info.isTouchFirst());
		info.formFactor = FormFactor::Phone;
		CHECK(info.isTouchFirst());
		info.formFactor = FormFactor::Desktop;
		CHECK(!info.isTouchFirst());
	}

	TEST(DescriptionNamesSystemFormFactorAndCapabilities) {
		PlatformInfo info;
		info.system = OperatingSystem::Android;
		info.systemVersion = "16";
		info.formFactor = FormFactor::Tablet;
		info.deviceName = "Google Pixel Tablet";
		info.contentScale = 2;
		info.screenSize = { 1280, 800 };
		info.hasTouch = true;
		info.hasPen = true;

		const std::string text = describe(info);

		CHECK(text.find("Android 16") != std::string::npos);
		CHECK(text.find("tablet Google Pixel Tablet") != std::string::npos);
		CHECK(text.find("scale 2.00") != std::string::npos);
		CHECK(text.find("screen 1280x800") != std::string::npos);
		CHECK(text.find("touch pen") != std::string::npos);
		CHECK(text.find("mouse") == std::string::npos);
	}
}
