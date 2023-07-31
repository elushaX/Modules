

#include "HeapAllocatorGlobal.hpp"
#include "PrivateConfig.hpp"

#include "Utils.hpp"
#include "Debugging.hpp"

#include <cstdio>
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
tp::Mutex tp::HeapAllocGlobal::mMutex;
bool tp::HeapAllocGlobal::mIgnore;

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
		uhalni mBlockSize;
		uhalni mIgnored;
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
	ALLOCATE:
	auto head = (MemHead*)malloc(aBlockSize + WRAP_SIZE * 2 + HEAD_SIZE);
	if (!head) {
		printf("WARNING : Cant allocate memory. Trying again\n");
		goto ALLOCATE; // Just freeze if no memory is available
	}

	auto wrap_top = (int1*)(head + 1);
	auto data = wrap_top + WRAP_SIZE;
	auto wrap_bottom = data + aBlockSize;

	head->mBlockSize = aBlockSize;
	head->mIgnored = mIgnore;

	// 2) Link with existing blocks
	mMutex.lock();
	mNumAllocations++;
	if (mEntry) {
		DEBUG_ASSERT(mEntry->mNext == nullptr)
		head->mNext = nullptr;
		head->mPrev = mEntry;
		mEntry->mNext = head;
	} else {
		head->mNext = nullptr;
		head->mPrev = nullptr;
	}
	mEntry = head;


	// 3) Trace the stack
	#ifdef MEM_STACK_TRACE
	head->mCallStack = gCSCapture->getSnapshot();
	#endif

	mMutex.unlock();

	// 4) Wrap fill
	memSetVal(wrap_top, WRAP_SIZE, WRAP_VAL);
	memSetVal(wrap_bottom, WRAP_SIZE, WRAP_VAL);

	// 5) clear data
#ifdef MEM_CLEAR_ON_ALLOC
	memSetVal(data, aBlockSize, CLEAR_ALLOC_VAL);
#endif

	return data;
}

void HeapAllocGlobal::deallocate(void* aPtr) {
	if (!aPtr) return;

	// 1) Restore the pointers
	auto head = ((MemHead*)((int1*)aPtr - WRAP_SIZE)) - 1;
	auto wrap_top = (int1*)(head + 1);
	auto data = wrap_top + WRAP_SIZE;
	auto wrap_bottom = data + head->mBlockSize;

	// 2) Unlink with blocks
	mMutex.lock();
	mNumAllocations--;
	DEBUG_ASSERT(!mEntry->mNext)
	if (head->mNext) head->mNext->mPrev = head->mPrev;
	if (head->mPrev) head->mPrev->mNext = head->mNext;
	if (head == mEntry) {
		mEntry = head->mPrev;
	}
	mMutex.unlock();

	if (!head->mIgnored) {
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
	}

	// 5) free the block
	free(head);
}

bool HeapAllocGlobal::checkLeaks() {
	ualni ignoredCount = 0;
	for (auto iter = mEntry; iter; iter = iter->mPrev) {
		ignoredCount += iter->mIgnored;
	}

	// 1) Check for not deallocated memory
	if (mNumAllocations && ignoredCount != mNumAllocations) {

		#ifdef MEM_STACK_TRACE
		for (auto iter = mEntry; iter; iter = iter->mPrev) {
			if (!iter->mIgnored) CallStackCapture::printSnapshot(iter->mCallStack);
		}
		#endif

		printf(" Count : %llu", mNumAllocations);

		ASSERT(!"Destruction of not freed Allocator")
		return true;
	}
	return false;
}

void HeapAllocGlobal::startIgnore() {
	mMutex.lock();
	mIgnore = true;
	mMutex.unlock();
}

void HeapAllocGlobal::stopIgnore() {
	mMutex.lock();
	mIgnore = false;
	mMutex.unlock();
}

HeapAllocGlobal::~HeapAllocGlobal() = default;

#endif

