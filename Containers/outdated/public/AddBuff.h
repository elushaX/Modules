
#pragma once

#include "common.h"

namespace tp {
	extern ModuleManifest gModuleContainer;

	template <typename Type, halni Step, halni Start=0, typename Index = halni, typename tAllocator>
	class AddBuffer {

    tAllocator mAlloc;
		Type* mBuff = NULL;
		Index mLoadIdx = 0;
		Index mLength = 0;

	public:

		AddBuffer() {
			MODULE_SANITY_CHECK(gModuleContainer);
			mLength = Step + Start;
			mBuff = new(mAlloc) [mLength];
		}

		void extend(Index aInputLen) {
			auto old = mBuff;
			auto old_len = mLength;

			mLength += (aInputLen / Step) + Step;
			mBuff = new(mAlloc) [mLength];

			for (auto i = 0; i < old_len; i++) {
				mBuff[i] = old[i];
			}

      delete[] (mAlloc, old);
		}

		void append(const Type* input, Index input_len) {
			if (mLoadIdx >= mLength) {
				extend(input_len);
			}

			for (auto i = 0; i < input_len; i++) {
				mBuff[mLoadIdx + i] = input[i];
			}

			mLoadIdx += input_len;
		}

		Type& operator[] (Index idx) {
			assert(idx >= 0 && idx <= mLoadIdx && "out of range");
			return mBuff[idx];
		}

		const Type* buff() const {
			return mBuff;
		}

		const Type& operator[] (Index idx) const {
			assert(idx >= 0 && idx <= mLoadIdx && "out of range");
			return mBuff[idx];
		}

		Index len() {
			return mLoadIdx;
		}

		Index lenReserved() {
			return mLength;
		}

		~AddBuffer() {
      delete[] (mAlloc, mBuff);
		}
	};
};