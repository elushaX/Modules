
/*
* 
Implementation:
* Adding chunk pointer to each chunk to form one-directional list that keeps track of free chunk
* Storing ordered pointers to chunks in order to find desired chunk from delete pointer on de-allocation in log time
* 
* Allocations:
* 1) allocate with chunk stored in list entry
* 2) ...
* 
* De-allocations:
* 1) binary-search with delete pointer to find desired chunk
* 2) ...
* 
*/

#include "Allocators.hpp"
#include "PrivateConfig.hpp"

void tp::PoolAlloc::Chunks::add(Chunk* aChunk) {
	// ensure order
	auto smaller_address = findUtil(mBuff, mBuff + mUsedLen, aChunk);
	for (auto iter = mBuff + mUsedLen; iter != smaller_address; iter--) { *(iter + 1) = *iter; }

	*(smaller_address + 1) = aChunk;
	mUsedLen++;

	// check for buff overflow
	if (mUsedLen == mLen) {
		auto prevBuff = mBuff;
		mBuff = (Chunk**) HeapAllocGlobal::allocate(sizeof(Chunk*) * mLen * 2);
		memcp(mBuff, prevBuff, sizeof(Chunk*) * mUsedLen);
		mLen *= 2;
		HeapAllocGlobal::deallocate(prevBuff);
	}
}

void tp::PoolAlloc::Chunks::remove(Chunk* aChunk) {
	// ensure order
	auto del_address = findUtil(mBuff, mBuff + mUsedLen, aChunk);
	for (auto iter = del_address; iter != mBuff + mUsedLen; iter++) { *iter = *(iter + 1); }
	
	mUsedLen--;

	// check for buff low usage
	if ((halnf)mUsedLen / (halnf)mLen < 0.25f) {
		auto prevBuff = mBuff;
		mBuff = (Chunk**)HeapAllocGlobal::allocate(sizeof(Chunk*) * mLen / 2);
		memcp(mBuff, prevBuff, sizeof(Chunk*) * mUsedLen);
		mLen /= 2;
		HeapAllocGlobal::deallocate(prevBuff);
	}
}

tp::PoolAlloc::Chunk* tp::PoolAlloc::Chunks::find(void* aPtr) {
	return *findUtil(mBuff, mBuff + mUsedLen, aPtr);
}

tp::PoolAlloc::Chunk* tp::PoolAlloc::Chunks::findNotFull() {
	for (ualni idx = 0; idx < mUsedLen; idx++) {
		if (!mBuff[idx]->isFull()) {
			return mBuff[idx];
		}
	}
	return nullptr;
}

tp::PoolAlloc::Chunk** tp::PoolAlloc::Chunks::findUtil(Chunk** aLeft, Chunk** aRight, void* aPtr) {
	auto range = ualni(aRight - aLeft);
	if (range == 1) { return aLeft; }
	auto middle = aLeft + range / 2;
	return (aPtr > *middle) ? findUtil(middle, aRight, aPtr) : findUtil(aLeft, middle, aPtr);
}


tp::PoolAlloc::PoolAlloc(ualni aBlockSize, ualni aChunkSize) : mBlockSize(aBlockSize), mChunkSize(aChunkSize) {}

void* tp::PoolAlloc::allocate() {

	if (!mFreeChunk || mFreeChunk->isFull()) {
		auto new_free_chunk = mChunks.findNotFull();
		
		if (!new_free_chunk) {

			new_free_chunk = new ((Chunk*)HeapAllocGlobal::allocate(sizeof(Chunk))) Chunk(mBlockSize, mChunkSize);
			ASSERT(new_free_chunk);
			mChunks.add(new_free_chunk);

			if (mFreeChunk) {
				new_free_chunk->mNext = mFreeChunk->mNext;
				new_free_chunk->mPrev = mFreeChunk->mPrev;
				if (mFreeChunk->mNext) mFreeChunk->mNext->mPrev = new_free_chunk;
				if (mFreeChunk->mPrev) mFreeChunk->mPrev->mNext = new_free_chunk;
			}
			else {
				new_free_chunk->mNext = nullptr;
				new_free_chunk->mPrev = nullptr;
			}

			mFreeChunk = new_free_chunk;
		}
	}

	return mFreeChunk->allocate();
}

void tp::PoolAlloc::deallocate(void* aPtr) {
	auto chunk = mChunks.find(aPtr);
	chunk->deallocate(aPtr);
	
	if (mFreeChunk->isEmpty()) {

		Chunk* new_chunk = nullptr;
		for (ualni idx = 0; idx < mChunks.mUsedLen; idx++) {
			if (!mChunks.mBuff[idx]->isFull() && new_chunk != mFreeChunk) {
				new_chunk = mChunks.mBuff[idx];
			}
		}

		if (new_chunk) {
			if (mFreeChunk->mNext) mFreeChunk->mNext->mPrev = mFreeChunk->mPrev;
			if (mFreeChunk->mPrev) mFreeChunk->mPrev->mNext = mFreeChunk->mNext;
			mChunks.remove(mFreeChunk);
			HeapAllocGlobal::deallocate(mFreeChunk);
			mFreeChunk = new_chunk;
		}
	}
}

tp::PoolAlloc::~PoolAlloc() = default;
