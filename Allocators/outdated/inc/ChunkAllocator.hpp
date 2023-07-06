#pragma once

#include "common.h"
#include "PublicConfig.hpp"

namespace tp {

	// Chunk Allocator
	// Constant time allocations and deallocations in any order.
	// Memory blocks are fixed in size and number of blocks can not exceed given parameter.
	struct ChunkAlloc {

		ChunkAlloc(ualni aBlockSize, void* aMemory, ualni aMemSize);
		ChunkAlloc(ualni aBlockSize, ualni aNBlocks);

		void* allocate();
		void deallocate(void* aPtr);
		bool isFull();
		bool isEmpty();

		~ChunkAlloc();

	private:
		ualni mBSize; // Size of data in aligned units
		ualni mNBlocks;

		ualni* mBuff;
		ualni* mNextBlock = 0;
		ualni mNFreeBlocks;
		ualni mNInitBlocks = 0;
		bool mOwnBuff = false;
	};
};