
#include "Module.hpp"

#include <UnitTest++/UnitTest++.h>
#include <UnitTest++/TestReporterStdout.h>

extern tp::ModuleManifest objTestModule;

#define TEST_OFF(name) static void name()