#pragma once

#include "common.h"
#include "PublicConfig.hpp"

namespace tp {

	struct HeapAllocGlobal {

#ifdef MEM_DEBUG
		static ualni mNumAllocations;
		static struct MemHead* mEntry;
#endif
		
		static void* allocate(ualni aBlockSize);
		static void deallocate(void* aPtr);
		~HeapAllocGlobal();
	};
};
