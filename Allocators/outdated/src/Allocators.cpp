
#include "allocators.hpp"

#include "filesystem.h"

static tp::ModuleManifest* sModuleDependencies[] = { &tp::gModuleFilesystem, NULL };
tp::ModuleManifest tp::gModuleAllocator = ModuleManifest("Allocators", NULL, NULL, sModuleDependencies);


//void* operator new(size_t aSize, void* aWhere) noexcept { return aWhere; }

void* operator new(size_t aSize) { return tp::HeapAllocGlobal::allocate(aSize); }
void* operator new[](size_t aSize) { return tp::HeapAllocGlobal::allocate(aSize); }
void  operator delete(void* aPtr) { tp::HeapAllocGlobal::deallocate(aPtr); }
void  operator delete[](void* aPtr) { tp::HeapAllocGlobal::deallocate(aPtr); }

void* operator new(size_t aSize, tp::HeapAlloc& aAlloc) { return aAlloc.allocate(aSize); }
void* operator new[](size_t aSize, tp::HeapAlloc& aAlloc) { return aAlloc.allocate(aSize); }
void  operator delete(void* aPtr, tp::HeapAlloc& aAlloc) { aAlloc.deallocate(aPtr); }
void  operator delete[](void* aPtr, tp::HeapAlloc& aAlloc) { aAlloc.deallocate(aPtr); }

void* operator new(size_t aSize, tp::HeapAllocGlobal& aAlloc) { return aAlloc.allocate(aSize); }
void* operator new[](size_t aSize, tp::HeapAllocGlobal& aAlloc) { return aAlloc.allocate(aSize); }
void  operator delete(void* aPtr, tp::HeapAllocGlobal& aAlloc) { aAlloc.deallocate(aPtr); }
void  operator delete[](void* aPtr, tp::HeapAllocGlobal& aAlloc) { aAlloc.deallocate(aPtr); }