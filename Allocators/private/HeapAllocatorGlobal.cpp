

#include "HeapAllocatorGlobal.hpp"
#include "PrivateConfig.hpp"

#include "Utils.hpp"
#include "Debugging.hpp"

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
		const CallStackCapture::CallStack* mCallStack;
		#endif
	};
}

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
		DEBUG_ASSERT(!mEntry->mNext)
		head->mNext = nullptr;
		head->mPrev = mEntry;
		mEntry->mNext = head;
	} else {
		head->mNext = nullptr;
		head->mPrev = nullptr;
	}
	mEntry = head;

	// 3) Wrap fill
	memSetVal(wrap_top, WRAP_SIZE, WRAP_VAL);
	memSetVal(wrap_bottom, WRAP_SIZE, WRAP_VAL);

	// 4) Trace the stack
	#ifdef MEM_STACK_TRACE
	head->mCallStack = gCSCapture->getSnapshot();
	#endif

	// 5) clear data
#ifdef MEM_CLEAR_ON_ALLOC
	memSetVal(data, aBlockSize, CLEAR_ALLOC_VAL);
#endif

	return data;
}

void HeapAllocGlobal::deallocate(void* aPtr) {
	// 1) Restore the pointers
	auto head = ((MemHead*)((int1*)aPtr - WRAP_SIZE)) - 1;
	auto wrap_top = (int1*)(head + 1);
	auto data = wrap_top + WRAP_SIZE;
	auto wrap_bottom = data + head->mBlockSize;

	// 2) Unlink with blocks
	mNumAllocations--;
	DEBUG_ASSERT(!mEntry->mNext)
	if (head->mNext) head->mNext->mPrev = head->mPrev;
	if (head->mPrev) head->mPrev->mNext = head->mNext;
	if (head == mEntry) {
		mEntry = head->mPrev;
	}

	// 3) Check the wrap
	if (memCompareVal(wrap_top, WRAP_SIZE, WRAP_VAL)) {
		CallStackCapture::printSnapshot(head->mCallStack);
		ASSERT(!"Allocated Block Wrap Corrupted!")
	}

	if (memCompareVal(wrap_bottom, WRAP_SIZE, WRAP_VAL)) {
		CallStackCapture::printSnapshot(head->mCallStack);
		ASSERT(!"Allocated Block Wrap Corrupted!")
	}

	// 4) clear data
#ifdef MEM_CLEAR_ON_ALLOC
	memSetVal(data, head->mBlockSize, CLEAR_DEALLOC_VAL);
#endif

	// 5) free the block
	free(head);
}

bool HeapAllocGlobal::checkLeaks() {
	// 1) Check for not deallocated memory
	if (mNumAllocations) {

		#ifdef MEM_STACK_TRACE
		for (auto iter = mEntry; iter; iter = iter->mPrev) {
			CallStackCapture::printSnapshot(iter->mCallStack);
		}
		#endif

		ASSERT(!"Destruction of not freed Allocator")
		return true;
	}
	return false;
}

HeapAllocGlobal::~HeapAllocGlobal() = default;

#endif

