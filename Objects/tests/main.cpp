
#include "ObjectTests.hpp"

#include "primitives/PrimitiveObjects.hpp"

tp::ModuleManifest* objDeps[] = { &tp::obj::gModuleObjects, nullptr };
tp::ModuleManifest objTestModule("ObjectsTests", nullptr, nullptr, objDeps);

#include <UnitTest++/UnitTest++.h>

int main(int /*argc*/, char** /*argv*/) { return UnitTest::RunAllTests(); }