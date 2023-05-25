
/*
* 
* Implementation uses embedded one-directional linked list to track free blocks.
* The embedded part ensures that there is no memory overhead on block specifically.
* Linked list is initialized iteratively on each allocation if it has not been already.

* Allocating:
* 1) updating list entry to stored in the entry itself next free pointer
* 2) returning entry before (1).
* 
* Deallocating: 
* 1) assigning list entry value to the deleted block
* 2) updating list entry to that block.
* 
*/

#include "ChunkAllocator.hpp"
#include "PrivateConfig.hpp"

#include "HeapAllocatorGlobal.hpp"

using namespace tp;

enum : ualni {
	ALIGNED_SIZE = ENV_ALNI_SIZE_B,
	WRAP_SIZE_ALN = MEM_WRAP_SIZE,
	WRAP_SIZE = WRAP_SIZE_ALN * ALIGNED_SIZE,
	WRAP_VAL = MEM_WRAP_FILL_VAL,
	CLEAR_ALLOC_VAL = MEM_CLEAR_ON_ALLOC_VAL,
	CLEAR_DEALLOC_VAL = MEM_CLEAR_ON_DEALLOC_VAL,
};

ChunkAlloc::ChunkAlloc(ualni aBlockSize, void* aMemory, ualni aMemSize) {
	auto const temp = aBlockSize / ALIGNED_SIZE;
	mBSize = ((aBlockSize % ALIGNED_SIZE) ? temp + 1 : temp) + WRAP_SIZE_ALN * 2;
	mBuff = (ualni*) aMemory;
	mNBlocks = (aMemSize / ALIGNED_SIZE) / mBSize;
	mNFreeBlocks = mNBlocks;
}

ChunkAlloc::ChunkAlloc(ualni aBlockSize, ualni aNBlocks) {
	auto const temp = aBlockSize / ALIGNED_SIZE;
	mBSize = ((aBlockSize % ALIGNED_SIZE) ? temp + 1 : temp) + WRAP_SIZE_ALN * 2;
	mNBlocks = aNBlocks;
	mNFreeBlocks = mNBlocks;
	mBuff = (ualni*)HeapAllocGlobal::allocate(mNBlocks * mBSize * ALIGNED_SIZE);
	mOwnBuff = true;
}

void* ChunkAlloc::allocate() {
	RelAssert(mNFreeBlocks && "Out Of Memory");

	// 1) PreInitialize blocks
	if (mNInitBlocks < mNBlocks) {
		*(mBuff + mNInitBlocks * mBSize) = (ualni)(mBuff + (mNInitBlocks++) * mBSize);
	}

	// 2) Find free block and update next free block
	auto data = mNextBlock;
	mNextBlock = (ualni*)(*data);
	mNFreeBlocks--;

	#ifdef MEM_DEBUG
		// 3) Fill Wrap and offset data
		auto wrap_top = data;
		data += WRAP_SIZE_ALN;
		auto wrap_bottom = data + mBSize;

		memsetv(wrap_top, WRAP_SIZE, WRAP_VAL);
		memsetv(wrap_bottom, WRAP_SIZE, WRAP_VAL);

		// 4) Clear data
		#ifdef MEM_CLEAR_ON_ALLOC
			memsetv(data, mBSize * ALIGNED_SIZE, CLEAR_ALLOC_VAL);
		#endif
	#endif

	return data;
}

void ChunkAlloc::deallocate(void* aPtr) {
	auto block = (ualni*)aPtr;

	#ifdef MEM_DEBUG
		// 3) Check Wrap and offset data
		auto wrap_bottom = block + mBSize;
		auto wrap_top = block - WRAP_SIZE_ALN;
		block = wrap_top;

		// 3) Check the wrap
		RelAssert(memequalv(wrap_top, WRAP_SIZE, WRAP_VAL) && memequalv(wrap_bottom, WRAP_SIZE, WRAP_VAL) && "Allocated Block Wrap Corrupted!");

		// 4) Clear data
		#ifdef MEM_CLEAR_ON_ALLOC
			memsetv(aPtr, mBSize * ALIGNED_SIZE, CLEAR_DEALLOC_VAL);
		#endif

	#endif

	(*block) = (ualni)mNextBlock;
	mNextBlock = block;
	mNFreeBlocks++;
}

bool ChunkAlloc::isFull() { return !mNFreeBlocks; }
bool ChunkAlloc::isEmpty() { return mNFreeBlocks == mNBlocks; }

ChunkAlloc::~ChunkAlloc() {
	if (mOwnBuff) {
		HeapAllocGlobal::deallocate(mBuff);
	}
}