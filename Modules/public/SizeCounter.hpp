#pragma once

#include "Common.hpp"

namespace tp {

	// TODO
	template <bool tRecursive>
	class SizeCounter {
		typedef SizeCounter& ArchRef;
		template <typename T, typename = void> struct HasSizeCounter : FalseType {};
		template <typename T> struct HasSizeCounter<T, VoidType<decltype(DeclareValue<T>()().count(DeclareValue<ArchRef>()()))>> : TrueType {};

	public:
		SizeCounter() = default;

		template<typename Type>
		void count(const Type& val) {
			if constexpr (HasSizeCounter<Type>::value) {
				val.count(*this);
			} else {
				if constexpr (tRecursive) {
					count(sizeof(val));
				} else {
					mSize += sizeof(Type);
				}
			}
		}

		void countSizeUnused(ualni size) {
			mSizeUnused += size;
		}

		void countSize(ualni size) {
			mSize += size;
		}

		[[nodiscard]] ualni getSizeUnused() const {
			return mSizeUnused;
		}

		[[nodiscard]] ualni getSize() const {
			return mSize;
		}

	private:
		ualni mSize = 0;
		ualni mSizeUnused = 0;
	};
}