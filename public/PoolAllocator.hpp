#pragma once

#include "ChunkAllocator.hpp"

namespace tp {

	// Pool Allocator
	// Overcomes chunk allocator fixed number of max allocations
	struct PoolAlloc {

		PoolAlloc(ualni aBlockSize, ualni aChunkSize);
		void* allocate();
		void deallocate(void* aPtr);
		~PoolAlloc();

	private:

		struct Chunk : public ChunkAlloc {
			Chunk(ualni aBlockSize, ualni aChunlSize) : ChunkAlloc(aBlockSize, aChunlSize) {}
			Chunk* mNext = NULL;
			Chunk* mPrev = NULL;
		};

		struct Chunks {
			void add(Chunk*);
			void remove(Chunk*);
			Chunk* find(void* aPtr);
			Chunk* findNotFull();
			Chunk** mBuff = NULL;
			ualni mUsedLen = 0;
			ualni mLen = 0;

		private:
			Chunk** findUtil(Chunk** aLeft, Chunk** aRight, void* aPtr);
		};

		Chunks mChunks;
		Chunk* mFreeChunk = NULL;
		ualni mBlockSize;
		ualni mChunkSize;
	};
};
