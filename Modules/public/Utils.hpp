#pragma once

#include "Module.hpp"

#define PTR_OFFSET(first, offset) (*((&first) + offset))
#define MEMBER_OFFSET(s, m) (alni(&(((s*) 0)->m)))

namespace tp {
	void memSetVal(void* p, uhalni byteSize, uint1 val);
	void memCopy(void* left, const void* right, uhalni len);
	int1 memCompare(const void* left, const void* right, uhalni len);
	bool memEqual(const void* left, const void* right, uhalni len);
	int1 memCompareVal(const void* left, uhalni len, uint1 val);
}

namespace tp {
	[[nodiscard]] alnf randomFloat();
}

namespace tp {

	template <typename Type = alni>
	class Bits {
		Type mFlags = 0;

	public:
		Bits() = default;

		explicit Bits(Type val) { mFlags = val; }

		explicit Bits(bool val) {
			for (int bit = 0; bit < sizeof(Type); bit++) {
				set(bit, val);
			}
		}

		bool get(int1 idx) { return mFlags & (1l << idx); }

		void set(int1 idx, bool val) {
			if (val) {
				mFlags |= (1l << idx);
			} else {
				mFlags &= ~(1l << idx);
			}
		}
	};

	template <typename tType = ualni>
	class Range {
	public:
		class Iterator {
		public:
			tType mIndex;

			explicit Iterator(tType pStartIndex) :
				mIndex(pStartIndex) {}

			tType index() const { return mIndex; }

			inline void operator++() { mIndex++; }

			inline operator tType() const { return mIndex; }

			inline bool operator==(Iterator pIndex) { return mIndex == pIndex.mIndex; }

			inline bool operator!=(Iterator pIndex) { return mIndex != pIndex.mIndex; }

			inline const Iterator& operator*() { return *this; }
		};

		tType mBegin{};
		tType mEnd{};

		Range() = default;

		explicit Range(tType pEndIndex) :
			mBegin(0),
			mEnd(pEndIndex) {}

		Range(tType pStartIndex, tType pEndIndex) :
			mBegin(pStartIndex),
			mEnd(pEndIndex) {}

		bool valid() const { return mBegin < mEnd; }

		tType idxBegin() const { return mBegin; }

		tType idxEnd() const { return mEnd; }

		tType idxDiff() const { return mEnd - mBegin; }

		Iterator begin() const { return Iterator(mBegin); }

		Iterator end() const { return Iterator(mEnd); }
	};
}
