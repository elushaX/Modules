
#include "Module.hpp"

int main() {
  tp::ModuleManifest* ModuleDependencies[] = { &tp::gModuleBase, nullptr };
  tp::ModuleManifest TestModule("Test", nullptr, nullptr, ModuleDependencies);

  if (!TestModule.initialize()) {
    return 1;
  }

  ASSERT(tp::gEnvironment.mWidth == tp::Environment::ArchWidth::X64);

  TestModule.deinitialize();
}