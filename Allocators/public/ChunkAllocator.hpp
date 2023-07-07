#pragma once

/*
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
*/

#include "Environment.hpp"
#include "PrivateConfig.hpp"

namespace tp {

	// Chunk Allocator
	// Constant time allocations and de-allocations in any order.
	// Memory blocks are fixed in size and number of blocks can not exceed given parameter.
	template<typename tType, ualni tNumBlocks>
	class ChunkAlloc {

		enum : ualni {
			ALIGNED_SIZE = ENV_ALNI_SIZE_B,

			WRAP_SIZE_ALN = MEM_WRAP_SIZE / 2,
			WRAP_SIZE = WRAP_SIZE_ALN * ALIGNED_SIZE,

			WRAP_VAL = MEM_WRAP_FILL_VAL,
			CLEAR_ALLOC_VAL = MEM_CLEAR_ON_ALLOC_VAL,
			CLEAR_DEALLOC_VAL = MEM_CLEAR_ON_DEALLOC_VAL,
		};

		static constexpr ualni dataSize() {
			auto BLOCK_SIZE_BYTES = sizeof(tType);
			auto BLOCK_SIZE_ALIGNED = BLOCK_SIZE_BYTES / ALIGNED_SIZE;
			return BLOCK_SIZE_ALIGNED;
		}

		static constexpr ualni blockSize() {
			auto BLOCK_SIZE_BYTES = sizeof(tType);
			auto BLOCK_SIZE = dataSize() + bool(BLOCK_SIZE_BYTES % ALIGNED_SIZE) + WRAP_SIZE_ALN * 2;
			return BLOCK_SIZE;
		}

	private:
		ualni* mNextBlock;
		ualni mNumFreeBlocks;
		ualni mNumInitBlocks;
		ualni mBuff[tNumBlocks * blockSize() * ALIGNED_SIZE];

	public:
		ChunkAlloc() {
			mNumFreeBlocks = tNumBlocks;
			mNumInitBlocks = 0;
			mNextBlock = mBuff;
		}

		~ChunkAlloc() = default; // TODO : check for leaks

	public:
		void* allocate(ualni) {
			DEBUG_ASSERT(mNumFreeBlocks && "Out Of Memory")

			// 1) PreInitialize blocks
			if (mNumInitBlocks < tNumBlocks) {
				mBuff[mNumInitBlocks * blockSize()] = (ualni) (mBuff + (mNumInitBlocks + 1) * blockSize());
				mNumInitBlocks++;
			}

			// 2) Find free block and update next free block
			auto data = mNextBlock;
			mNextBlock = (ualni*)(*data);
			mNumFreeBlocks--;

			#ifdef MEM_DEBUG
				// 3) Fill Wrap and offset data
				auto wrap_top = data;
				auto wrap_bottom = data + WRAP_SIZE_ALN + dataSize();

				memSetVal(wrap_top, WRAP_SIZE, WRAP_VAL);
				memSetVal(wrap_bottom, WRAP_SIZE, WRAP_VAL);

				// 4) Clear data
				#ifdef MEM_CLEAR_ON_ALLOC
					memSetVal(data + WRAP_SIZE_ALN, dataSize() * ALIGNED_SIZE, CLEAR_ALLOC_VAL);
				#endif

				data += WRAP_SIZE_ALN;
			#endif

			return data;
		}

		void deallocate(void* aPtr) {
			DEBUG_ASSERT(aPtr >= mBuff && aPtr < mBuff + tNumBlocks * blockSize())

			auto block = (ualni*)aPtr;

			#ifdef MEM_DEBUG
				// 3) Check Wrap and offset data
				auto wrap_bottom = block + dataSize();
				auto wrap_top = block - WRAP_SIZE_ALN;
				block = wrap_top;

				// 3) Check the wrap
				ASSERT(!memCompareVal(wrap_top, WRAP_SIZE, WRAP_VAL) && "Allocated Block Wrap Corrupted!")
				ASSERT(!memCompareVal(wrap_bottom, WRAP_SIZE, WRAP_VAL) && "Allocated Block Wrap Corrupted!")

				// 4) Clear data
				#ifdef MEM_CLEAR_ON_ALLOC
					memSetVal(block, blockSize() * ALIGNED_SIZE, CLEAR_DEALLOC_VAL);
				#endif

			#endif

			(*block) = (ualni)mNextBlock;
			mNextBlock = block;
			mNumFreeBlocks++;
		}

		[[nodiscard]] bool checkWrap() const { return false; }
		void checkValid() {}

	public:
		[[nodiscard]] bool isFull() const { return !mNumFreeBlocks; }
		[[nodiscard]] bool isEmpty() const { return mNumFreeBlocks == tNumBlocks; }
		[[nodiscard]] const ualni* getBuff() const { return mBuff; }
	};
}