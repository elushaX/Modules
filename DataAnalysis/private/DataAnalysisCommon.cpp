#include "DataAnalysisCommon.hpp"
#include "ContainersCommon.hpp"
#include "MathCommon.hpp"

namespace tp {
  static ModuleManifest* deps[] = {&gModuleMath, &gModuleContainers, nullptr};
  ModuleManifest gModuleDataAnalysis = ModuleManifest("DataAnalysis", nullptr, nullptr, deps);
}
