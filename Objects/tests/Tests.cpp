
#include "NewPlacement.hpp"
#include "Testing.hpp"

#include "primitives/primitives.h"

using namespace tp;
using namespace obj;


void testCore();
void testPrimitives();
void testInterpreter();

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {
		testCore();
		module.deinitialize();
	}

	if (module.initialize()) {
		testPrimitives();
		module.deinitialize();
	}

	if (module.initialize()) {
		testInterpreter();
		module.deinitialize();
	}

	return 0;
}