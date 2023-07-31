
#include "NewPlacement.hpp"
#include "Testing.hpp"

#include "primitives/primitives.h"

using namespace tp;
using namespace obj;


void testCore();
void testPrimitives();

int main() {

	tp::ModuleManifest* deps[] = { &gModuleObjects, nullptr };
	tp::ModuleManifest module("ObjectsTests", nullptr, nullptr, deps);

	if (module.initialize()) {

		testCore();
		testPrimitives();

		module.deinitialize();
	}
	return 0;
}