#pragma once

#include "HeapAllocatorGlobal.hpp"

namespace tp {

	struct PickAlloc {
		PickAlloc();
		void* allocate(ualni aBlockSize);
		void deallocate(void* aPtr);
		~PickAlloc();
	};
};
