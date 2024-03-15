
#include "UnitTest++/UnitTest++.h"

#include "Debugging.hpp"
#include "Utils.hpp"

#include <cstdio>

using namespace tp;

void printSnapshot(const tp::CallStackCapture::CallStack* snapshot) {
	printf("CallStack: \n");
	for (auto frame : *snapshot) {
		auto symbols = gCSCapture->getSymbols(frame.getFrame());
		printf("  %s   -----   %s:%llu\n", symbols->getFunc(), symbols->getFile(), symbols->getLine());
	}
	printf("\n");
}

void common() { auto tmp = gCSCapture->getSnapshot(); }

void first() {
	common();
	common();
	common();
}

void second() {
	common();
	common();
	common();
	common();
}

void third() {
	common();
	common();
}

void root() {
	first();
	second();
	third();
}

SUITE(Utils) {
	TEST(CallStackCapture) {
		tp::ModuleManifest* deps[] = { &tp::gModuleUtils, nullptr };
		tp::ModuleManifest testModule("UtilsTest", nullptr, nullptr, deps);

		REQUIRE CHECK(testModule.initialize());

		root();

		gCSCapture->logAll();

		testModule.deinitialize();
	}

	TEST(CAllStackCaptureContent) { 
		CHECK(false);
	}
}

int main() { return UnitTest::RunAllTests(); }