
#include "Testing.hpp"

#include "primitives/primitives.h"

using namespace tp;
using namespace obj;

void testParser();
void testCore();
void testPrimitives();
void testInterpreter();

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {

		// testParser();
		// testCore();
		// testPrimitives();
		testInterpreter();

		module.deinitialize();
	}

	return 0;
}
