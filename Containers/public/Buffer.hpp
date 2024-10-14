
#pragma once

#include "ContainersCommon.hpp"

namespace tp {

	template <ualni tScale = 2>
	inline ualni BufferResizeScaling(const ualni size) {
		return size * tScale;
	}

	template <ualni tScale = 2>
	inline ualni BufferResizeScalingDown(const ualni size) {
		return size / tScale;
	}

	template <ualni tAddition = 1>
	inline ualni BufferResizeAddition(const ualni size) {
		return size + tAddition;
	}

	template <ualni tAddition = 1>
	inline ualni BufferResizeAdditionDown(const ualni size) {
		return size - tAddition;
	}

	template <typename tType, ualni tSize>
	class ConstSizeBuffer {
		typedef SelectValueOrReference<tType> Arg;

	private:
		tType mBuff[tSize];
		ualni mLoad = 0;

	public:
		ConstSizeBuffer() = default;

		~ConstSizeBuffer() {
			for (auto i = 0; i < mLoad; i++) {
				mBuff[i].~tType();
			}
		}

	public:
		[[nodiscard]] ualni size() const { return mLoad; }

		[[nodiscard]] ualni getBuffSize() const { return tSize; }

		tType& first() {
			DEBUG_ASSERT(mLoad)
			return *mBuff;
		}

		const tType& first() const {
			DEBUG_ASSERT(mLoad)
			return *mBuff;
		}

		tType& last() {
			DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1];
		}

		const tType& last() const {
			DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1];
		}

		tType& operator[](ualni aIdx) {
			DEBUG_ASSERT(aIdx >= 0 && aIdx < mLoad)
			return mBuff[aIdx];
		}

		const tType& operator[](ualni aIdx) const {
			DEBUG_ASSERT(aIdx >= 0 && aIdx < mLoad)
			return mBuff[aIdx];
		}

		void append(Arg data) {
			ASSERT(mLoad != tSize)
			new (&mBuff[mLoad]) tType(data);
			mLoad++;
		}

		void pop() {
			DEBUG_ASSERT(mLoad)
			mBuff[mLoad].~tType();
			mLoad--;
		}

		ConstSizeBuffer& operator=(const tp::InitialierList<tType>& init) {
			for (auto arg : init) {
				append(arg);
			}
			return *this;
		}

		void clear() {
			this->~ConstSizeBuffer();
			new (this) ConstSizeBuffer();
		}

	public:
		class IteratorPointer {
		protected:
			tType* mIter;

		public:
			IteratorPointer() = default;

			tType& operator->() { return *mIter; }

			const tType& operator->() const { return *mIter; }
		};

		class IteratorReference {
		protected:
			tType* mIter;

		public:
			IteratorReference() = default;

			tType* operator->() { return mIter; }

			const tType* operator->() const { return mIter; }
		};

		class Iterator : public TypeSelect<TypeTraits<tType>::isPointer, IteratorPointer, IteratorReference>::Result {
		public:
			explicit Iterator(tType* iter) { this->mIter = iter; }

			const Iterator& operator*() const { return *this; }

			Iterator& operator++() {
				this->mIter++;
				return *this;
			}

			bool operator==(const Iterator& left) const { return left.mIter == this->mIter; }

			bool operator!=(const Iterator& left) const { return left.mIter != this->mIter; }
		};

		[[nodiscard]] Iterator begin() const { return Iterator(mBuff); }

		[[nodiscard]] Iterator end() const { return Iterator(mBuff + mLoad); }

	public:
		template <class tArchiver>
		void archiveWrite(tArchiver& ar) const {
			ar << mLoad;
			for (auto item : *this) {
				ar << item.data();
			}
		}

		template <class tArchiver>
		void archiveRead(tArchiver& ar) {
			clear();
			ar >> mLoad;
			for (auto i = 0; i < mLoad; i++) {
				ar >> mBuff[i];
			}
		}
	};

	template <
		typename tType,
		class tAllocator = DefaultAllocator,
		ualni(tResizePolicy)(ualni) = BufferResizeScaling<2>,
		ualni(tResizePolicyDown)(ualni) = BufferResizeScalingDown<2>,
		ualni tMinSize = 4>
	class Buffer {
		typedef SelectValueOrReference<tType> Arg;

	private:
		tType* mBuff;
		tAllocator mAllocator;
		ualni mSize;
		ualni mLoad;

	public:
		Buffer() :
			mSize(tMinSize),
			mLoad(0) {
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * tMinSize);
		}

		Buffer(const InitialierList<tType>& input) {
			mSize = 0;
			for (const auto& val : input) {
				mSize++;
			}
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * mSize);
			mLoad = 0;
			for (const auto& val : input) {
				mBuff[mLoad] = val;
				mLoad++;
			}
		}

		explicit Buffer(ualni size) {
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * size);
			mSize = size;
			mLoad = size;
			for (ualni i = 0; i < mLoad; i++) {
				new (mBuff + i) tType();
			}
		}

		Buffer(const Buffer& in) :
			mSize(in.mSize),
			mLoad(in.mLoad) {
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * mSize);
			for (ualni i = 0; i < mLoad; i++) {
				new (&mBuff[i]) tType(in.mBuff[i]);
			}
		}

		void clear() {
			this->~Buffer();
			new (this) Buffer();
		}

		Buffer& operator=(const Buffer& in) {
			if (this == &in) {
				return *this;
			}
			this->~Buffer();
			new (this) Buffer(in);
			return *this;
		}

		~Buffer() {
			for (ualni i = 0; i < mLoad; i++) {
				mBuff[i].~tType();
			}
			mAllocator.deallocate(mBuff);
		}

	public:
		[[nodiscard]] ualni size() const { return mLoad; }

		[[nodiscard]] ualni getBuffSize() const { return mSize; }

		[[nodiscard]] const tType* getBuff() const { return mBuff; }

		[[nodiscard]] tType* getBuff() { return mBuff; }

		tType& first() {
			DEBUG_ASSERT(mLoad)
			return *mBuff;
		}

		const tType& first() const {
			DEBUG_ASSERT(mLoad)
			return *mBuff;
		}

		tType& last() {
			DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1];
		}

		const tType& last() const {
			DEBUG_ASSERT(mLoad)
			return mBuff[mLoad - 1];
		}

		tType& operator[](ualni idx) {
			DEBUG_ASSERT(idx < mLoad && idx >= 0)
			return mBuff[idx];
		}

		const tType& operator[](ualni idx) const {
			DEBUG_ASSERT(idx < mLoad && idx >= 0)
			return mBuff[idx];
		}

	public:
		bool operator==(const Buffer& in) const {
			if (this == &in) {
				return true;
			}
			if (mLoad != in.mLoad) {
				return false;
			}
			for (ualni i = 0; i < mLoad; i++) {
				if (mBuff[i] != in.mBuff[i]) {
					return false;
				}
			}
			return true;
		}

	public:
		Buffer& operator=(const tp::InitialierList<tType>& input) {
			// TODO : optimize
			for (ualni i = 0; i < mLoad; i++) {
				mBuff[i].~tType();
			}
			mAllocator.deallocate(mBuff);

			mSize = 0;
			for (const auto& val : input) {
				mSize++;
			}

			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * mSize);
			mLoad = 0;
			for (const auto& val : input) {
				new (&mBuff[mLoad]) tType(val);
				mLoad++;
			}

			return *this;
		}

		alni find(const tType& val) {
			for (ualni idx = 0; idx < mLoad; idx++) {
				if (mBuff[idx] == val) return (alni) idx;
			}
			return -1;
		}

		tType& append(Arg data) {
			if (mLoad == mSize) {
				resizeBuffer(tResizePolicy(mSize));
			}
			new (&mBuff[mLoad]) tType(data);
			mLoad++;
			return mBuff[mLoad - 1];
		}

		void append(const Buffer& in) {
			if (!in.mLoad) {
				return;
			}
			auto newLoad = mLoad + in.mLoad;
			auto newSize = mSize;
			while (newLoad >= newSize) {
				newSize = tResizePolicy(newSize);
			}
			if (newSize != mSize) {
				resizeBuffer(newSize);
			}
			for (auto i = mLoad; i < newLoad; i++) {
				new (&mBuff[i]) tType(in.mBuff[i - mLoad]);
			}
			mLoad = newLoad;
		}

		void pop() {
			DEBUG_ASSERT(mLoad)
			mBuff[mLoad].~tType();
			mLoad--;
			ualni prevSize = tResizePolicyDown(mSize);
			DEBUG_ASSERT(prevSize < mSize)
			if (prevSize > mLoad) {
				resizeBuffer(prevSize);
			}
		}

		void reserve(ualni aSize) {
			for (ualni i = 0; i < mLoad; i++) {
				mBuff[i].~tType();
			}
			mAllocator.deallocate(mBuff);
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * aSize);
			mSize = aSize;
			mLoad = aSize;
			for (ualni i = 0; i < mLoad; i++) {
				new (mBuff + i) tType();
			}
		}

		void erase(ualni start, ualni end) {
			DEBUG_ASSERT(end <= mLoad)
			DEBUG_ASSERT(end >= start)

			if (start == end) return;

			for (ualni idx = start; idx < end; idx++) {
				mBuff[idx].~tType();
			}

			const auto diff = (end - start);
			for (ualni idx = end; idx < mLoad; idx++) {
				new (&mBuff[idx - diff]) tType(mBuff[idx]);
				mBuff[idx].~tType();
			}

			mLoad -= diff;
			ualni prevSize = tResizePolicyDown(mSize);
			DEBUG_ASSERT(prevSize < mSize)
			if (prevSize > mLoad) {
				resizeBuffer(prevSize);
			}
		}

		template<typename tRemoveConditionFunctor>
		void erase_if(tRemoveConditionFunctor functor) {
			alni lastIndex = mLoad - 1;
			alni currentIndex = 0;
			while (currentIndex < lastIndex + 1) {
				if (functor(mBuff[currentIndex])) {
					new (&mBuff[currentIndex]) tType(mBuff[lastIndex]);
					mBuff[lastIndex].~tType();
					lastIndex--;
				} else {
					currentIndex++;
				}
			}

			erase(lastIndex + 1, mLoad);
		}

		void reverse() {
			for (ualni idx = 0; idx < mLoad / 2; idx++) {
				swapV(mBuff[idx], mBuff[mLoad - idx - 1]);
			}
		}

	public:
		class IteratorPointer {
		protected:
			tType* mIter;

		public:
			IteratorPointer() = default;

			tType& operator->() { return *mIter; }

			const tType& operator->() const { return *mIter; }
		};

		class IteratorReference {
		protected:
			tType* mIter;

		public:
			IteratorReference() = default;

			tType* operator->() { return mIter; }

			const tType* operator->() const { return mIter; }
		};

		class Iterator : public TypeSelect<TypeTraits<tType>::isPointer, IteratorPointer, IteratorReference>::Result {
		public:
			explicit Iterator(tType* iter) { this->mIter = iter; }

			const Iterator& operator*() const { return *this; }

			tType& data() { return *this->mIter; }

			Iterator& operator++() {
				this->mIter++;
				return *this;
			}

			bool operator==(const Iterator& left) const { return left.mIter == this->mIter; }

			bool operator!=(const Iterator& left) const { return left.mIter != this->mIter; }
		};

		[[nodiscard]] Iterator begin() const { return Iterator(mBuff); }

		[[nodiscard]] Iterator end() const { return Iterator(mBuff + mLoad); }

	public:
		template <class tArchiver>
		void archiveWrite(tArchiver& ar) const {
			ar << mLoad;
			for (auto item : *this) {
				ar % item.data();
			}
		}

		template <class tArchiver>
		void archiveRead(tArchiver& ar) {
			clear();
			decltype(mLoad) len;
			ar >> len;
			for (auto i = len; i; i--) {
				// TODO : optimize
				if (mLoad == mSize) {
					resizeBuffer(tResizePolicy(mSize));
				}
				new (mBuff + mLoad) tType();
				ar % mBuff[mLoad];
				mLoad++;
			}
		}

	private:
		void resizeBuffer(ualni newSize) {
			DEBUG_ASSERT(newSize >= mLoad)
			const auto oldBuff = mBuff;
			mBuff = (tType*) mAllocator.allocate(sizeof(tType) * newSize);
			for (ualni i = 0; i < mLoad; i++) {
				new (&mBuff[i]) tType(oldBuff[i]);
			}
			for (ualni i = 0; i < mLoad; i++) {
				oldBuff[i].~tType();
			}
			mAllocator.deallocate(oldBuff);
			mSize = newSize;
		}
	};

	template <typename tType>
	void generatePermutations(const Buffer<Buffer<tType>>& in, Buffer<Buffer<tType>>& out) {
		typedef long long Idx;

		// sanity check
		for (const auto& vec : in) {
			if (!vec.size()) {
				return;
			}
		}

		out.resize(in.size());
		auto len = Idx(1);
		for (const auto& vec : in) {
			len *= (Idx) vec.size();
		}
		for (auto i = 0; i < in.size(); i++) {
			out[i].resize(len);
		}

		auto dub = Idx(1);
		for (auto power = (Idx) in.size() - 1; power >= 0; power--) {
			auto& vec = in[power];
			long long i = 0;
			while (i < len) {
				for (auto val : vec) {
					for (auto j = 0; j < dub; j++) {
						out[power][i] = val;
						i++;
					}
				}
			}
			dub *= (Idx) vec.size();
		}
	}

}
