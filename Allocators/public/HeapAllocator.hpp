#pragma once

#include "Environment.hpp"

namespace tp {

	struct HeapAlloc {

#ifdef MEM_DEBUG
		ualni mNumAllocations = 0;
		struct MemHeadLocal* mEntry = nullptr;
#endif

		void* allocate(ualni aBlockSize);
		void deallocate(void* aPtr);
		~HeapAlloc();
	};
};
