

#include "HeapAllocator.hpp"
#include "HeapAllocatorGlobal.hpp"

#include "PrivateConfig.hpp"

#include <stddef.h>
#include <cstdlib>

using namespace tp;

#if not defined(MEM_DEBUG)

// ----------------------- Release Implementation ---------------------------- //
void* HeapAlloc::allocate(ualni aBlockSize) { return malloc(aBlockSize); }
void HeapAlloc::deallocate(void* aPtr) { free(aPtr); }
HeapAlloc::~HeapAlloc() {}

#else

namespace tp {
	struct MemHeadLocal {
		MemHeadLocal* mPrev;
		MemHeadLocal* mNext;
	};
};

void* HeapAlloc::allocate(ualni aBlockSize) {
	auto head = (MemHeadLocal*) HeapAllocGlobal::allocate(aBlockSize + sizeof(MemHeadLocal));
	auto out = head + 1;

	mNumAllocations++;
	if (mEntry) {
		head->mNext = mEntry->mNext;
		head->mPrev = mEntry->mPrev;
		if (mEntry->mNext) mEntry->mNext->mPrev = head;
		if (mEntry->mPrev) mEntry->mPrev->mNext = head;
	}
	else {
		head->mNext = nullptr;
		head->mPrev = nullptr;
	}
	mEntry = head;

	return out;
}

void HeapAlloc::deallocate(void* aPtr) {
	auto head = ((MemHeadLocal*)(aPtr)) - 1;
	
	mNumAllocations--;
	if (mEntry->mNext) mEntry->mNext->mPrev = mEntry->mPrev;
	if (mEntry->mPrev) mEntry->mPrev->mNext = mEntry->mNext;
	if (head == mEntry) {
		if (mEntry->mNext) {
			mEntry = mEntry->mNext;
		}
		else {
			mEntry = mEntry->mPrev;
		}
	}

	HeapAllocGlobal::deallocate(head);
}

HeapAlloc::~HeapAlloc() {
	if (mNumAllocations) {
		DEBUG_BREAK("Destruction of not freed Allocator");

		#ifdef MEM_STACK_TRACE
		// TODO : log leaks and free them up
		#endif
	}
}

#endif

