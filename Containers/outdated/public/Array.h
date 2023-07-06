#pragma once

#include "Cassert.hpp"
#include "Sort.h"
#include "Filesystem.hpp"

namespace tp {
	extern ModuleManifest gModuleContainer;

	template <typename Type>
	class ArrayIterator;

	template <typename Type, typename tAllocator>
	class Array {

		void allocate(alni p_bufflen) {
			mLength = p_bufflen;
			mBuff = new(mAlloc.Allocate(mLength * sizeof(Type))) Type[mLength]();
		}

    void free() {
      if (mBuff) {
        mLength = 0;
        delete[] (mAlloc, mBuff);
        mBuff = NULL;
      }
    }

    tAllocator mAlloc;
		Type* mBuff;
		alni mLength;

		public:

		Array() {
			MODULE_SANITY_CHECK(gModuleContainer);
			mLength = 0;
			mBuff = nullptr;
		}

		Array(tp::init_list<Type> list) {
			mLength = 0;
			mBuff = nullptr;
			this->operator=(list);
		}

		Array(alni p_length) {
			mLength = p_length;
			mBuff = nullptr;
			reserve(mLength);
		}

		alni length() const {
			return mLength;
		}

		Type* buff() const {
			return mBuff;
		}

		void reserve(alni p_bufflen) {
			free();
      allocate(alni p_bufflen);
		}

		void insert(Type& p_block, alni idx) {
			Type* current = mBuff;
			allocate(mLength + 1);

			for (alni befor = 0; befor < idx; befor++) {
				mBuff[befor] = current[befor];
			}
			for (alni after = idx; after < mLength - 1; after++) {
				mBuff[after + 1] = current[after];
			}

			mBuff[idx] = p_block;

			if (current) {
				delete[] (mAlloc, current);
			}
		}

		void remove(alni p_idx) {
			Type* current = mBuff;
			allocate(mLength - 1);

			for (alni befor = 0; befor < p_idx; befor++) {
				mBuff[befor] = current[befor];
			}
			for (alni after = p_idx + 1; after < mLength + 1; after++) {
				mBuff[after - 1] = current[after];
			}

      delete[] (mAlloc, current);
		}

		void removeVal(Type val) {
			for (int i = 0; i < mLength; i++) {
				if (mBuff[i] == val) {
					remove(i);
				}
			}
		}

		Type& last() {
			return mBuff[mLength - 1];
		}

		void extend(tp::ualni new_len) {
			if ((tp::ualni)mLength >= new_len) {
				return;
			}
				
			if (!mBuff) {
				reserve(new_len);
			}

			auto old_buff = mBuff;
			auto old_len = (tp::ualni) mLength;

			mBuff = new Type[new_len]();
			mLength = new_len;

			for (tp::ualni idx = 0; idx < old_len; idx++) {
				mBuff[idx] = old_buff[idx];
			}

      delete[] (mAlloc, old_buff);
		}

		void operator=(const Array& array) {
			reserve(array.mLength);
			for (int i = 0; i < array.mLength; i++) {
				mBuff[i] = array.mBuff[i];
			}
		}

		void operator+=(const Array& array) {
			if (!mBuff) {
				return operator=(array);
			}

			alni new_len = array.mLength + mLength;
			Type* newbuff = new Type[new_len]();

			for (halni i = 0; i < mLength; i++) {
				newbuff[i] = mBuff[i];
			}
			for (halni i = 0; i < array.mLength; i++) {
				newbuff[mLength + i] = array.mBuff[i];
			}

      delete[] (mAlloc, mBuff);
			mBuff = newbuff;
			mLength = new_len;
		}

		void operator=(tp::init_list<Type> list) {
			reserve(list.size());
			uhalni idx = 0;
			for (Type item : list) {
				mBuff[idx] = item;
				idx++;
			}
		}

		inline Type& operator[](alni idx) {
			assert(idx >= 0 && idx < mLength);
			return mBuff[idx];
		}


		inline const Type& operator[](alni idx) const {
			assert(idx >= 0 && idx < mLength);
			return mBuff[idx];
		}

		void pushBack(Type block) {
			insert(block, mLength);
		}

		Array(const Array& array) {
			allocate(array.mLength);
			for (int i = 0; i < array.mLength; i++) {
				mBuff[i] = array.mBuff[i];
			}
		}

		ArrayIterator<Type> begin() { return ArrayIterator<Type>(this); }
		alni end() { return mLength; }

		alni saveSize() {
			return mLength * sizeof(Type) + sizeof(mLength);
		}

		void save(File& file) {
			file.write(&mLength);
			file.write_bytes((int1*) mBuff, mLength * sizeof(Type));
		}

		void load(File& file) {
			file.read(&mLength);
			reserve(mLength);
			file.read_bytes((int1*) mBuff, mLength * sizeof(Type));
		}

		~Array() {
			free();
		}

		alni sizeAllocatedMem() {
			alni out = 0;
			out += sizeof(alni);
			out += sizeof(Type*);
			if (mBuff) {
				out += sizeof(Type) * mLength;
			}
			return out;
		}

		template <typename SortPolicy = SortMerge>
		void sort(bool (*grater)(Type const& item1, Type const& item2)) {
			SortPolicy sorter;
			sorter.sort(mBuff, (int) mLength, grater);
		}

		alni sizeUsedMem() {
			return sizeAllocatedMem();
		}
	};

	template <typename Type>
	class ArrayIterator {
		alni mIdx = 0;
		Array<Type>* mArrayPtr;
		public:

		ArrayIterator(Array<Type>* array) : mArrayPtr(array) {}

		Type& data() const { return (*mArrayPtr)[mIdx]; }
		ualni idx() const { return mIdx; }
		operator alni() { return mIdx; }

		Type* operator->() { return &(*mArrayPtr)[mIdx]; }
		const ArrayIterator& operator*() { return *this; }

		inline void operator++() { mIdx++; }
		bool operator==(alni p_idx) { return mIdx == p_idx; }
		bool operator!=(alni p_idx) { return mIdx != p_idx; }
		bool operator>(alni p_idx) { return mIdx > p_idx; }
		bool operator<(alni p_idx) { return mIdx < p_idx; }
		bool operator>=(alni p_idx) { return mIdx >= p_idx; }
		bool operator<=(alni p_idx) { return mIdx <= p_idx; }
	};

};