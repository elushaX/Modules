
#include "ObjectTests.hpp"

using namespace tp;
using namespace obj;

void testParser();
void testCore();
void testPrimitives();
void testInterpreter();

tp::ModuleManifest* objDeps[] = { &gModuleObjects, nullptr };
tp::ModuleManifest objTestModule("ObjectsTests", nullptr, nullptr, objDeps);

int main() {

	testParser();
	testCore();
	testPrimitives();
	testInterpreter();

	return 0;
}
