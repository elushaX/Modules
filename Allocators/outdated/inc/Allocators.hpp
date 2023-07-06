#pragma once

#include "HeapAllocator.hpp"
#include "ChunkAllocator.hpp"
#include "PoolAllocator.hpp"

namespace tp {
	extern ModuleManifest gModuleAllocator;
};

inline void* operator new(size_t aSize, void* aWhere) noexcept { return aWhere; }

void* operator new(size_t aSize);
void* operator new[](size_t _Size);
void  operator delete(void* aPtr);
void  operator delete[](void* aPtr);

void* operator new(size_t aSize, tp::HeapAlloc& aAlloc);
void* operator new[](size_t _Size, tp::HeapAlloc& aAlloc);
void  operator delete(void* aPtr, tp::HeapAlloc& aAlloc);
void  operator delete[](void* aPtr, tp::HeapAlloc& aAlloc);

void* operator new(size_t aSize, tp::HeapAllocGlobal& aAlloc);
void* operator new[](size_t _Size, tp::HeapAllocGlobal& aAlloc);
void  operator delete(void* aPtr, tp::HeapAllocGlobal& aAlloc);
void  operator delete[](void* aPtr, tp::HeapAllocGlobal& aAlloc);