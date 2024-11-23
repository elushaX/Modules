
#pragma once

#include "Buffer.hpp"
#include "Utils.hpp"

namespace tp {

	template <typename tType, ualni tSizeX, ualni tSizeY>
	using ConstSizeBuffer2D = ConstSizeBuffer<ConstSizeBuffer<tType, tSizeX>, tSizeY>;
	
	typedef ualni Index;

	struct Index2D {
		Index x = 0;
		Index y = 0;
	};

	template <typename tType, class tAllocator = DefaultAllocator>
	class Buffer2D {
	public:

		typedef SelectValueOrReference<tType> tTypeArg;

	private:
		tAllocator mAlloc;
		tType* mBuff = nullptr;
		Index2D mSize = { 0, 0 };

		void deleteBuffer() {
			if (!mBuff) {
				return;
			}
			for (ualni i = 0; i < mSize.x * mSize.y; i++) {
				mBuff[i].~tType();
			}
			mAlloc.deallocate(mBuff);
		}

		void allocateBuffer(Index2D size) {
			deleteBuffer();
			mBuff = (tType*) mAlloc.allocate(sizeof(tType) * size.x * size.y);
			for (ualni i = 0; i < size.x * size.y; i++) {
				new (mBuff + i) tType();
			}
		}

	public:
		Buffer2D() = default;

		~Buffer2D() {
			deleteBuffer();
			mSize = { 0, 0 };
		}

		explicit Buffer2D(Index2D aSize) { reserve(aSize); }

		[[nodiscard]] Index2D size() const { return { mSize.x, mSize.y }; }

		tType* getBuff() const { return mBuff; }
    void setBuff(tType* data, Index2D size) { mBuff = data; mSize = size; }

		void flipY() {
			for (Index i = 0; i < mSize.x; i++) {
				const auto lenIdx = mSize.y - 1;
				for (Index j = 0; j < mSize.y / 2; j++) {
					swapV(get({ i, j }), get({ i, lenIdx - j }));
				}
			}
		}

		void flipX() {
			for (Index i = 0; i < mSize.y; i++) {
				const auto lenIdx = mSize.x - 1;
				for (Index j = 0; j < mSize.x / 2; j++) {
					swapV(get({ i, j }), get({ i, lenIdx - j }));
				}
			}
		}

		inline tType& get(const Index2D& at) {
			DEBUG_ASSERT(mBuff && at.x < mSize.x && at.y < mSize.y && at.x >= 0 && at.y >= 0)
			return *(mBuff + mSize.x * at.y + at.x);
		}

		inline const tType& get(const Index2D& at) const {
			DEBUG_ASSERT(mBuff && at.x < mSize.x && at.y < mSize.y && at.x >= 0 && at.y >= 0)
			return *(mBuff + mSize.x * at.y + at.x);
		}

		inline void set(const Index2D& at, tTypeArg value) {
			DEBUG_ASSERT(mBuff && at.x < mSize.x && at.y < mSize.y && at.x >= 0 && at.y >= 0)
			*(mBuff + mSize.x * at.y + at.x) = value;
		}

		void reserve(const Index2D& newSize) {
			if (mSize.x != newSize.x || mSize.y != newSize.y) {
				allocateBuffer(newSize);
				mSize = newSize;
			}
		}

		void assign(tType value) {
			DEBUG_ASSERT(mBuff);
			Index len = mSize.x * mSize.y;
			for (Index i = 0; i < len; i++) {
				mBuff[i] = value;
			}
		}

		bool operator==(const Buffer2D& in) const {
			if (&in == this) {
				return true;
			}
			if (in.size() != size()) {
				return false;
			}
			for (auto i = 0; i < mSize.x * mSize.y; i++) {
				if (mBuff[i] != in.mBuff[i]) {
					return false;
				}
			}
			return true;
		}

	public:
		template <class tArchiver>
		void archiveWrite(tArchiver& ar) const {
			ar << mSize;
			for (auto i = 0; i < mSize.x; i++) {
				for (auto j = 0; j < mSize.y; j++) {
					ar << get(i, j);
				}
			}
		}

		template <class tArchiver>
		void archiveRead(tArchiver& ar) {
			decltype(mSize) size;
			deleteBuffer();
			ar >> size;
			reserve(size);
			for (auto i = 0; i < mSize.x; i++) {
				for (auto j = 0; j < mSize.y; j++) {
					ar >> get(i, j);
				}
			}
		}
	};
}
