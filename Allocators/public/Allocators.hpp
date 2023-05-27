#pragma once

#include "BaseModule.hpp"

#include "HeapAllocatorGlobal.hpp"
#include "HeapAllocator.hpp"
#include "ChunkAllocator.hpp"
#include "PoolAllocator.hpp"

namespace tp {
	extern ModuleManifest gModuleAllocator;
};

inline void* operator new(std::size_t aSize, void* aWhere) noexcept { return aWhere; }

void* operator new(std::size_t aSize);
void* operator new[](std::size_t aSize);
void  operator delete(void* aPtr);
void  operator delete[](void* aPtr);

void* operator new(std::size_t aSize, tp::HeapAlloc& aAlloc);
void* operator new[](std::size_t aSize, tp::HeapAlloc& aAlloc);
void  operator delete(void* aPtr, tp::HeapAlloc& aAlloc);
void  operator delete[](void* aPtr, tp::HeapAlloc& aAlloc);

void* operator new(std::size_t aSize, tp::HeapAllocGlobal& aAlloc);
void* operator new[](std::size_t aSize, tp::HeapAllocGlobal& aAlloc);
void  operator delete(void* aPtr, tp::HeapAllocGlobal& aAlloc);
void  operator delete[](void* aPtr, tp::HeapAllocGlobal& aAlloc);