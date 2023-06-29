#pragma once

#include "Module.hpp"

#define PTR_OFFSET(first, offset) (*((&first) + offset))
#define MEMBER_OFFSET(s, m) (alni(&(((s*)0)->m)))

namespace tp {

  extern ModuleManifest gModuleUtils;

  void memsetv(void* p, uhalni bytesize, uint1 val);
  void memcp(void* left, const void* right, uhalni len);
  int1 memequal(const void* left, const void* right, uhalni len);
}

namespace tp {
  [[nodiscard]] alnf randf();
}

namespace tp {

  template <typename T1, typename T2>
  class Pair {
  public:
    Pair() {}
    Pair(T1 t1, T2 t2) : head(t1), tail(t2) {}
    union { T1 t1; T1 head; T1 x; };
    union { T2 t2; T2 tail;	T2 y; };
  };

  template <typename Type = alni>
  class Bits {
    Type mFlags = 0;
  public:
    Bits() = default;
    explicit Bits(Type val) { mFlags = val; }
    explicit Bits(bool val) { for (int bit = 0; bit < sizeof(Type); bit++) { set(bit, val); } }
    bool get(int1 idx) { return mFlags & (1l << idx); }
    void set(int1 idx, bool val) {
      if (val) {
        mFlags |= (1l << idx);
      } else {
        mFlags &= ~(1l << idx);
      }
    }
  };


  template<typename tType = ualni>
  class Range {
  public:
    class Iterator {
    public:
      tType mIndex;
      explicit Iterator(tType pStartIndex) : mIndex(pStartIndex) {}
      tType index() const { return mIndex; }
      inline void operator++() { mIndex++; }
      inline operator tType() const { return mIndex; }
      inline bool operator==(Iterator pIndex) { return mIndex == pIndex.mIndex; }
      inline bool operator!=(Iterator pIndex) { return mIndex != pIndex.mIndex; }
      inline const Iterator& operator*() { return *this; }
    };

    tType mBegin = 0;
    tType mEnd = 0;

    Range() = default;
    explicit Range(tType pEndIndex) : mBegin(0), mEnd(pEndIndex) {}
    Range(tType pStartIndex, tType pEndIndex) : mBegin(pStartIndex), mEnd(pEndIndex) {}

    bool valid() { return mBegin < mEnd; }

    tType idxBegin() const { return mBegin; }
    tType idxEnd() const { return mEnd; }
    Iterator begin() { return Iterator(mBegin); }
    Iterator end() { return Iterator(mEnd); }
  };
}