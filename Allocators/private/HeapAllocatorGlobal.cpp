

#include "HeapAllocatorGlobal.hpp"
#include "PrivateConfig.hpp"

#include "Debugging.hpp"

#include <stddef.h>
#include <cstdlib>

using namespace tp;

#if not defined(MEM_DEBUG)

// ----------------------- Release Implementation ---------------------------- //

void* HeapAllocGlobal::allocate(ualni aBlockSize) { return malloc(aBlockSize); }
void HeapAllocGlobal::deallocate(void* aPtr) { free(aPtr); }
HeapAllocGlobal::~HeapAllocGlobal() = default;

#else

tp::MemHead* tp::HeapAllocGlobal::mEntry = nullptr;
tp::ualni tp::HeapAllocGlobal::mNumAllocations = 0;

// ----------------------- Debug Implementation ---------------------------- //
// |----------------|
// |     MemHead    |
// |----------------|
// |    wrap top    |
// |----------------|   - Allocated Block Layout
// |      data      |
// |----------------|
// |  wrap bottom   |
// |----------------|

namespace tp {
	struct MemHead {
		MemHead* mPrev;
		MemHead* mNext;
		ualni mBlockSize;
		#ifdef MEM_STACK_TRACE
		CallStackSnapshots::StackShapshot mCallStack;
		#endif
	};
};

enum : ualni {
	ALIGNED_SIZE = ENV_ALNI_SIZE_B,
	WRAP_SIZE = MEM_WRAP_SIZE * ALIGNED_SIZE,
	WRAP_VAL = MEM_WRAP_FILL_VAL,
	HEAD_SIZE = sizeof(MemHead),
	CLEAR_ALLOC_VAL = MEM_CLEAR_ON_ALLOC_VAL,
	CLEAR_DEALLOC_VAL = MEM_CLEAR_ON_DEALLOC_VAL,
};

void* HeapAllocGlobal::allocate(ualni aBlockSize) {

	static_assert(HEAD_SIZE % ALIGNED_SIZE == 0, "Heap Allocator Configuration Error");

	if (aBlockSize % ALIGNED_SIZE) {
		aBlockSize = (aBlockSize / ALIGNED_SIZE + 1) * ALIGNED_SIZE;
	}

	// 1) Allocate the block
	auto head = (MemHead*)malloc(aBlockSize + WRAP_SIZE * 2 + HEAD_SIZE);
	auto wrap_top = (int1*)(head + 1);
	auto data = wrap_top + WRAP_SIZE;
	auto wrap_bottom = data + aBlockSize;

	if (!head) { return nullptr; }
	head->mBlockSize = aBlockSize;

	// 2) Link with existing blocks
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

	// 3) Wrap fill
	memsetv(wrap_top, WRAP_SIZE, WRAP_VAL);
	memsetv(wrap_bottom, WRAP_SIZE, WRAP_VAL);

	// 4) Trace the stack
	#ifdef MEM_STACK_TRACE
	head->mCallStack = gCallStackSnapshots.capture();
	#endif

	// 5) clear data
#ifdef MEM_CLEAR_ON_ALLOC
	memsetv(data, aBlockSize, CLEAR_ALLOC_VAL);
#endif

	return data;
}

void HeapAllocGlobal::deallocate(void* aPtr) {
	// 1) Restore the pointers
	auto head = ((MemHead*)(aPtr)) - 1;
	auto wrap_top = (int1*)(head + 1);
	auto data = wrap_top + WRAP_SIZE;
	auto wrap_bottom = data + head->mBlockSize;

	// 2) Unlink with blocks
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

	// 3) Check the wrap
	RelAssert(memequalv(wrap_top, WRAP_SIZE, WRAP_VAL) && memequalv(wrap_bottom, WRAP_SIZE, WRAP_VAL) && "Allocated Block Wrap Corrupted!");

	// 4) clear data
#ifdef MEM_CLEAR_ON_ALLOC
	memsetv(data, head->mBlockSize, CLEAR_DEALLOC_VAL);
#endif

	// 5) free the block
	free(aPtr); 
}

HeapAllocGlobal::~HeapAllocGlobal() {
	// 1) Check for not deallocated memory
	if (mNumAllocations) {
		DEBUG_BREAK("Destruction of not freed Allocator");

		#ifdef MEM_STACK_TRACE
		// TODO: log leaks
		#endif
	}
}

#endif

