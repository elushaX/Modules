#pragma once

#include "HeapAllocatorGlobal.hpp"

namespace tp {

	struct HeapAlloc {

#ifdef MEM_DEBUG
		HeapAllocGlobal mAlloc;

		ualni mNumAllocations = 0;
		struct MemHeadLocal* mEntry = NULL;
#endif

		void* allocate(ualni aBlockSize);
		void deallocate(void* aPtr);
		~HeapAlloc();
	};
};
