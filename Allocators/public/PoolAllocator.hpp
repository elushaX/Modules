#pragma once

/*
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

#include "ChunkAllocator.hpp"

namespace tp {

	// Pool Allocator
	// Overcomes chunk allocator fixed number of max allocations
	template<typename tType, ualni tNumBlocks>
	class PoolAlloc {

		typedef ChunkAlloc<tType, tNumBlocks> Chunk;

		struct Chunks {

			void add(Chunk* aChunk){

				if (!mBuff) {
					mLen = 16;
					mBuff = (Chunk**) HeapAllocGlobal::allocate(sizeof(Chunk*) * mLen);
					mUsedLen = 1;
					mBuff[0] = aChunk;
					return;
				}

				// ensure order
				auto smaller_address = findUtil(mBuff, mBuff + mUsedLen, aChunk);
				for (auto iter = mBuff + mUsedLen; iter != smaller_address; iter--) {
					*iter = *(iter - 1);
				}

				*(smaller_address) = aChunk;
				mUsedLen++;

				// check for buff overflow
				if (mUsedLen == mLen) {
					auto prevBuff = mBuff;
					mBuff = (Chunk**) HeapAllocGlobal::allocate(sizeof(Chunk*) * mLen * 2);
					memCopy(mBuff, prevBuff, sizeof(Chunk*) * mUsedLen);
					mLen *= 2;
					HeapAllocGlobal::deallocate(prevBuff);
				}
			}

			void remove(Chunk** del_address){
				if (mUsedLen == 1) {
					mLen = 0;
					mUsedLen = 0;
					HeapAllocGlobal::deallocate(mBuff);
					mBuff = nullptr;
					return;
				}

				// ensure order
				for (auto iter = del_address; iter != mBuff + mUsedLen - 1; iter++) {
					*iter = *(iter + 1);
				}

				mUsedLen--;

				// check for buff low usage
				if ((halnf)mUsedLen / (halnf)mLen < 0.25f) {
					auto prevBuff = mBuff;
					mBuff = (Chunk**)HeapAllocGlobal::allocate(sizeof(Chunk*) * mLen / 2);
					memCopy(mBuff, prevBuff, sizeof(Chunk*) * mUsedLen);
					mLen /= 2;
					HeapAllocGlobal::deallocate(prevBuff);
				}
			}

			[[nodiscard]] Chunk** find(void* aPtr) {
				return findUtil(mBuff, mBuff + mUsedLen, aPtr) - 1;
			}

			[[nodiscard]] Chunk* findNotFull() const {
				for (ualni idx = 0; idx < mUsedLen; idx++) {
					if (!mBuff[idx]->isFull()) {
						return mBuff[idx];
					}
				}
				return nullptr;
			}

			Chunk** mBuff = nullptr;
			ualni mUsedLen = 0;
			ualni mLen = 0;

		private:
			Chunk** findUtil(Chunk** aLeft, Chunk** aRight, void* aPtr) {
				auto range = ualni(aRight - aLeft);
				if (range == 1) {
					return (aPtr < *aLeft) ? aLeft : aRight;
				}
				auto middle = aLeft + range / 2;
				return (aPtr >= (*middle)) ? findUtil(middle, aRight, aPtr) : findUtil(aLeft, middle, aPtr);
			}
		};

	private:
		Chunks mChunks;
		Chunk* mFreeChunk = nullptr;

	public:
		PoolAlloc() = default;
		~PoolAlloc() = default;

	public:
		void* allocate(ualni) {
			if (!mFreeChunk || mFreeChunk->isFull()) {
				auto new_free_chunk = mChunks.findNotFull();
				if (!new_free_chunk) {
					new_free_chunk = new (HeapAllocGlobal::allocate(sizeof(Chunk))) Chunk();
					DEBUG_ASSERT(new_free_chunk)
					mChunks.add(new_free_chunk);
				}
				mFreeChunk = new_free_chunk;
			}
			return mFreeChunk->allocate(0);
		}

		void deallocate(void* aPtr) {
			auto chunk = mChunks.find(aPtr);
			(*chunk)->deallocate(aPtr);
			if ((*chunk)->isEmpty()) {
        if (mFreeChunk == *chunk) mFreeChunk = nullptr;
        HeapAllocGlobal::deallocate(*chunk);
        mChunks.remove(chunk);
			}
		}

	public:
		[[nodiscard]] bool checkWrap() const { return false; }

		void checkValid() {
			return;
			for (auto i = 0; i < mChunks.mUsedLen; i++) {
				for (auto j = 0; j < mChunks.mUsedLen; j++) {
					if (i > j) {
						ASSERT(mChunks.mBuff[i] > mChunks.mBuff[j])
					} else if (i < j) {
					  ASSERT(mChunks.mBuff[i] < mChunks.mBuff[j])
					}
				}
			}
		}
	};
}
