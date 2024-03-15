

#include "HeapAllocator.hpp"
#include "HeapAllocatorGlobal.hpp"

#include "PrivateConfig.hpp"

#include <malloc.h>

using namespace tp;

#if not defined(MEM_DEBUG)

// ----------------------- Release Implementation ---------------------------- //
void* HeapAlloc::allocate(ualni aBlockSize) { return malloc(aBlockSize); }
void HeapAlloc::deallocate(void* aPtr) {
	if (!aPtr) return;
	free(aPtr);
}
HeapAlloc::~HeapAlloc() {}

#else

namespace tp {
	struct MemHeadLocal {
		MemHeadLocal* mPrev;
		MemHeadLocal* mNext;
	};
}

void* HeapAlloc::allocate(ualni aBlockSize) {
	auto head = (MemHeadLocal*) HeapAllocGlobal::allocate(aBlockSize + sizeof(MemHeadLocal));
	auto out = head + 1;

	mNumAllocations++;
	if (mEntry) {
		DEBUG_ASSERT(!mEntry->mNext)
		head->mNext = nullptr;
		head->mPrev = mEntry;
		mEntry->mNext = head;
	} else {
		head->mNext = nullptr;
		head->mPrev = nullptr;
	}
	mEntry = head;

	return out;
}

void HeapAlloc::deallocate(void* aPtr) {
	if (!aPtr) return;

	auto head = ((MemHeadLocal*) (aPtr)) - 1;

	mNumAllocations--;
	DEBUG_ASSERT(!mEntry->mNext)
	if (head->mNext) head->mNext->mPrev = head->mPrev;
	if (head->mPrev) head->mPrev->mNext = head->mNext;
	if (head == mEntry) {
		mEntry = head->mPrev;
	}

	HeapAllocGlobal::deallocate(head);
}

HeapAlloc::~HeapAlloc() {
	if (mNumAllocations) {
		DEBUG_ASSERT(0 && "Destruction of not freed Allocator")

#ifdef MEM_STACK_TRACE
		// TODO : log leaks and free them up
#endif
	}
}

#endif
