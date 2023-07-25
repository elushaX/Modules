#pragma once

#include "Common.hpp"

namespace tp {

	// Used to transfer data to or from some sort of archive
	// Abstracts interface so that actual archive must only overload readBytes and writeBytes
	// The main idea is that it transfers data as bytes only if this type of data does not have special API functions
	template <bool tRead>
	class ArchiverTemplate {
	public:
		struct HasFunc {
			typedef ArchiverTemplate& ArchRef;
			template <typename T, typename = void> struct Write : FalseType {};
			template <typename T> struct Write<T, VoidType<decltype(DeclareValue<T>()().archiveWrite(DeclareValue<ArchRef>()()))>> : TrueType {};

			template <typename T, typename = void> struct Read : FalseType {};
			template <typename T> struct Read<T, VoidType<decltype(DeclareValue<T>()().archiveRead(DeclareValue<ArchRef>()()))>> : TrueType {};

			template <typename T, typename = void> struct Archive : FalseType {};
			template <typename T> struct Archive<T, VoidType<decltype(DeclareValue<T>()().archive(DeclareValue<ArchRef>()()))>> : TrueType {};

			template <typename T>
			struct AssertCombinations {
				static constexpr auto assert() {
					if (HasFunc::template Read<T>::value != HasFunc::template Write<T>::value) return false;
					if (HasFunc::template Read<T>::value) {
						if (HasFunc::template Read<T>::value == HasFunc::template Archive<T>::value) return false;
					}
					return true;
				}
			};
		};

	public:
		virtual void writeBytes(const int1* val, ualni size) = 0;
		virtual void readBytes(int1* val, ualni size)  = 0;

	public:
		ArchiverTemplate() = default;

		// check if type has explicit write method. if not write as bytes
		template<typename Type>
		void operator <<(const Type& val) {
			static_assert(!tRead);
			static_assert(HasFunc::template AssertCombinations<Type>::assert());
			if constexpr (HasFunc::template Write<Type>::value) {
				val.archiveWrite(*this);
			} else {
				writeBytes((const int1*) &val, sizeof(val));
			}
		}

		// check if type has explicit read method. if not read as bytes
		template<typename Type>
		void operator >>(Type& val) {
			static_assert(tRead);
			static_assert(HasFunc::template AssertCombinations<Type>::assert());
			if constexpr (HasFunc::template Read<Type>::value) {
				val.archiveRead(*this);
			} else {
				readBytes((int1*) &val, sizeof(val));
			}
		}

		// check if type has explicit archive method. if not read/write as bytes
		template<typename Type>
		void operator %(Type& val) {
			static_assert(HasFunc::template AssertCombinations<Type>::assert());
			if constexpr (HasFunc::template Archive<Type>::value) {
				val.archive(*this);
			} else {
				if constexpr (tRead) {
					operator>>(val);
				} else {
					operator<<(val);
				}
			}
		}

		template<typename Type>
		void operator %(const Type& val) {
			static_assert(HasFunc::template AssertCombinations<Type>::assert());
			if constexpr (HasFunc::template Archive<Type>::value) {
				((Type&)val).archive(*this);
			} else {
				if constexpr (tRead) {
					operator>>(val);
				} else {
					operator<<(val);
				}
			}
		}
	};

	template<ualni tMaxMemory, bool tRead>
	class ArchiverExample : public ArchiverTemplate<tRead> {
	public:
		ArchiverExample() = default;

	protected:
		void incrementAddresses(ualni size) {
			if (mFirstNotWritten == mAddress) mFirstNotWritten += size;
			mAddress += size;
		}

		void writeBytes(const int1* val, ualni size) override {
			for (auto i = 0; i < size; i++) mBuff[mAddress + i] = val[i];
			incrementAddresses(size);
		}

		void readBytes(int1* val, ualni size) override {
			for (auto i = 0; i < size; i++) val[i] = mBuff[mAddress + i];
			incrementAddresses(size);
		}

		template<typename tType>
		static void test() {
			const tType val;
			tType res;

			res.change();

			ArchiverExample<tMaxMemory, false> write;
			ArchiverExample<tMaxMemory, true> read;

			write % val;

			for (auto i = 0; i < tMaxMemory; i++) read.mBuff[i] = write.mBuff[i];

			read % res;

			if (val != res) {
				throw "test failed";
			}
		}

	public:
		int1 mBuff[tMaxMemory]{};

	private:
		ualni mAddress = 0;
		ualni mFirstNotWritten = 0;
	};
}