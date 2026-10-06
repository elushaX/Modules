#pragma once

#include "Environment.hpp"

namespace tp {

	class HeapAlloc {

#ifdef MEM_DEBUG
		ualni mNumAllocations = 0;
		struct MemHeadLocal* mEntry = nullptr;
#endif

	public:
		HeapAlloc() = default;
		~HeapAlloc();

	public:
		void* allocate(ualni aBlockSize);
		void deallocate(void* aPtr);

	public:
		[[nodiscard]] bool checkWrap() const { return false; }
		void checkValid() {}
	};
}
