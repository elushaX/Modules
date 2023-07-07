#pragma once

#include "Module.hpp"

namespace tp {

	class HeapAllocGlobal {

#ifdef MEM_DEBUG
		static ualni mNumAllocations;
		static struct MemHead* mEntry;
#endif

	public:
		HeapAllocGlobal() = default;
		~HeapAllocGlobal();

	public:
		static void* allocate(ualni aBlockSize);
		static void deallocate(void* aPtr);

		static bool checkLeaks();
	public:
		[[nodiscard]] bool checkWrap() const { return false; }
		void checkValid() {}
	};
}
