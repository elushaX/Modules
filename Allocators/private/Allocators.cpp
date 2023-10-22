
#include "Allocators.hpp"

#include <cstdlib>

static void deinit(const tp::ModuleManifest* self) { tp::HeapAllocGlobal::checkLeaks(); }

static tp::ModuleManifest* sModuleDependencies[] = { &tp::gModuleUtils, nullptr };
tp::ModuleManifest tp::gModuleAllocators = ModuleManifest("Allocators", nullptr, deinit, sModuleDependencies);

void* operator new(size_t aSize) { return tp::HeapAllocGlobal::allocate(aSize); }
void* operator new[](size_t aSize) { return tp::HeapAllocGlobal::allocate(aSize); }
void operator delete(void* aPtr) noexcept { tp::HeapAllocGlobal::deallocate(aPtr); }
void operator delete[](void* aPtr) noexcept { tp::HeapAllocGlobal::deallocate(aPtr); }

void* operator new(size_t aSize, tp::HeapAlloc& aAlloc) { return aAlloc.allocate(aSize); }
void* operator new[](size_t aSize, tp::HeapAlloc& aAlloc) { return aAlloc.allocate(aSize); }
void operator delete(void* aPtr, tp::HeapAlloc& aAlloc) { aAlloc.deallocate(aPtr); }
void operator delete[](void* aPtr, tp::HeapAlloc& aAlloc) { aAlloc.deallocate(aPtr); }

void* operator new(size_t aSize, tp::HeapAllocGlobal& aAlloc) { return tp::HeapAllocGlobal::allocate(aSize); }
void* operator new[](size_t aSize, tp::HeapAllocGlobal& aAlloc) { return tp::HeapAllocGlobal::allocate(aSize); }
void operator delete(void* aPtr, tp::HeapAllocGlobal& aAlloc) { tp::HeapAllocGlobal::deallocate(aPtr); }
void operator delete[](void* aPtr, tp::HeapAllocGlobal& aAlloc) { tp::HeapAllocGlobal::deallocate(aPtr); }