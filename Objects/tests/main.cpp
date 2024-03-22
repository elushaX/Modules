
#include "ObjectTests.hpp"

#include "primitives/primitives.h"

tp::ModuleManifest* objDeps[] = { &obj::gModuleObjects, nullptr };
tp::ModuleManifest objTestModule("ObjectsTests", nullptr, nullptr, objDeps);

#include <UnitTest++/UnitTest++.h>

int main(int /*argc*/, char** /*argv*/) { return UnitTest::RunAllTests(); }