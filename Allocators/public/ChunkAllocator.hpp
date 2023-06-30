#pragma once

#include "Environment.hpp"

namespace tp {

	// Chunk Allocator
	// Constant time allocations and de-allocations in any order.
	// Memory blocks are fixed in size and number of blocks can not exceed given parameter.
	struct ChunkAlloc {

		ChunkAlloc(ualni aBlockSize, void* aMemory, ualni aMemSize);
		ChunkAlloc(ualni aBlockSize, ualni aNBlocks);

		void* allocate();
		void deallocate(void* aPtr);
		[[nodiscard]] bool isFull() const;
		[[nodiscard]] bool isEmpty() const;

		~ChunkAlloc();

	private:
		ualni mBSize; // Size of data in aligned units
		ualni mNBlocks;

		ualni* mBuff = nullptr;
		ualni* mNextBlock = nullptr;
		ualni mNFreeBlocks;
		ualni mNInitBlocks = 0;
		bool mOwnBuff = false;
	};
};