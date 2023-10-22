
#include "Tests.hpp"

#include "Strings.hpp"

void testStringLogic();
void testStrings();
void testLogging();

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleStrings, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("StringsTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	testStringLogic();
	testStrings();
	testLogging();

	testModule.deinitialize();
}