# Profiling

## Memory Leaks

Example program with memory leaks:

```c++
#include "allocators.h"

void test_call22() { new int; }
void test_call21() { new float; }

void test_call11() {
  test_call21();
  test_call22();
}

int main(char argc, char* argv[]) {
  tp::ModuleManifest* ModuleDependencies[] = { &tp::gModuleAllocator, NULL };
  tp::ModuleManifest TestModule("Test", NULL, NULL, ModuleDependencies);
  TestModule.initialize();
  
  test_call11();
  
  TestModule.deinitialize();
}

```
If memory leaks were detected it will be loged in the output console.

![image](https://user-images.githubusercontent.com/63184036/222794298-3f238de4-c0b8-41fa-b7ec-c0c675da8f05.png)

Also debug.memleaks binary will be generated in the working directory that can be viewved with MemLeaks Viewver.

![image](https://user-images.githubusercontent.com/63184036/222793169-a405effe-72be-42fc-b375-bb06dce0a735.png)


## Memory Usage Analisys
Currently outdated

## Benchmarks
Currently outdated
