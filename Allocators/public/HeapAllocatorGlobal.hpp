#pragma once

#include "Module.hpp"
#include <mutex>

namespace tp {

	class HeapAllocGlobal {

#ifdef MEM_DEBUG
		static ualni mNumAllocations;
		static struct MemHead* mEntry;
		static std::mutex mMutex;
		static bool mIgnore;
#endif

	public:
		HeapAllocGlobal() = default;
		~HeapAllocGlobal();

	public:
		static void* allocate(ualni aBlockSize);
		static void deallocate(void* aPtr);

		static bool checkLeaks();
		static void startIgnore();
		static void stopIgnore();
		static ualni getNAllocations();
		
	public:
		[[nodiscard]] bool checkWrap() const { return false; }
		void checkValid() {}
	};
}
