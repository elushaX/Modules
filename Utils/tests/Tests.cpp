
#include "Utils.hpp"
#include "Debugging.hpp"
#include "Testing.hpp"

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

void common() {
	gCSCapture->getSnapshot();
}

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

TEST_DEF(Debugging) {
	root();

	gCSCapture->logLeaks();
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("UtilsTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testDebugging();

	testModule.deinitialize();
}