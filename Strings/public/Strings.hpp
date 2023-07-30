#pragma once

#include "Allocators.hpp"
#include "StringLogic.hpp"

namespace tp {

	// Static data
	extern ModuleManifest gModuleStrings;

	template<typename tChar> class StringTemplate;

	// Hidden slave class
	template<typename tChar = int1>
	class StringData {
		friend StringTemplate<tChar>;
		typedef StringLogic<tChar> Logic;

	private:
		uint2 mIsConst; // source is non-modifiable
		uint2 mEditedIdx; // editor id
		uint4 mReferenceCount; // number of users of this
		tChar* mBuff; // actual string data

		StringData() {
			mBuff = (tChar*) "*";
			mReferenceCount = 1;
			mIsConst = true;
			mEditedIdx = 0;
		}

		StringData(const tChar* aBuff, bool aIsConst) {
			MODULE_SANITY_CHECK(gModuleStrings)
			DEBUG_ASSERT(aBuff)
			mBuff = (tChar*) aBuff;
			mReferenceCount = 0;
			mIsConst = aIsConst;
			mEditedIdx = 0;
		}

		explicit StringData(const tChar* aBuff) {
			MODULE_SANITY_CHECK(gModuleStrings)
			DEBUG_ASSERT(aBuff)
			auto const len = Logic::calcLength(aBuff);
			mReferenceCount = 0;
			mIsConst = false;
			mEditedIdx = 0;
			mBuff = new tChar[len + 1];
			mBuff[len] = Logic::getEndChar();
			memCopy(mBuff, aBuff, len);
		}

		[[nodiscard]] tChar* getBuffer() {
			return mBuff;
		}

		~StringData() {
			if (!mIsConst) delete[] mBuff;
		}

		void resize(ualni size) {
			DEBUG_ASSERT(mReferenceCount == 1 && !mIsConst)
			delete[] mBuff;
			mBuff = new tChar[size + 1];
			mBuff[size] = Logic::getEndChar();
		}
	};

	template<typename tChar = int1>
	class StringTemplate {
	public:
		typedef StringLogic<tChar> Logic;
		typedef StringData<tChar> Data;
		typedef typename Logic::Index Index;

		enum {
			STRINGS_POOL_SIZE = 1024,
			MAX_INT_STRING_LENGTH = 10,
			MAX_BOOL_STRING_LENGTH = 10,
			MAX_FLOAT_STRING_LENGTH = 10,
		};
		static PoolAlloc<StringData<tChar>, STRINGS_POOL_SIZE> sStringPool;
		static Data sNullString;

	private:
		Data* mData;

	public: //  Main Interface
		// Creates null string reference
		StringTemplate() {
			mData = &sNullString;
			incReference(mData);
		}

		// References existing string data
		StringTemplate(const StringTemplate& in) {
			mData = in.mData;
			incReference(mData);
		}

		// Creates new string data from raw data pointer.
		// Does not own string buffer - string buffer will not be freed. Initializes as const memory.
		StringTemplate(const tChar* raw) {
			mData = new (sStringPool.allocate(0)) StringData(raw, true);
			incReference(mData);
		}

		// Copies raw data and claims ownership over it
		StringTemplate(tChar* raw) {
			mData = new (sStringPool.allocate(0)) StringData(raw);
			incReference(mData);
		}

		~StringTemplate() {
			decReference(mData);
		}

	private:
		static void incReference(Data* dp) {
			dp->mReferenceCount++;
		}

		void decReference(Data* dp) {
			dp->mReferenceCount--;
			if (!dp->mReferenceCount) {
				mData->~StringData();
				sStringPool.deallocate(mData);
			}
		}

	public: // Access
		// used to read data
		[[nodiscard]] const tChar* read() const {
			return mData->mBuff;
		}

		// used to write data
		// output will be null if in TextEditing mode
		[[nodiscard]] tChar* write() {
			makeModifiable();
			return mData->getBuffer();
		}

		// output will be null if in TextEditing mode
		tChar* resize(uint1 size) {
			makeModifiable();
			mData->resize(size);
			return write();
		}

	public:
		void makeModifiable() {
			// We have no rights to modify if someone references that memory too
			// So we need to create new copy of StringData
			if (mData->mReferenceCount > 1) {
				mData = new (sStringPool.allocate(0)) StringData(mData->mBuff);
				incReference(mData);
				return;
			}

			// Also if initial raw data was marked as const - create copy of raw data
			// Note - raw data will be lost at this point if no other record of such is stored
			if (mData->mIsConst) {
				mData = new (sStringPool.allocate(0)) StringData(mData->mBuff);
				incReference(mData);
			}
		}

		template<class tArchiver>
		void archiveWrite(tArchiver& ar) const {
			auto size = this->size();
			ar << size;
			ar.writeBytes(read(), size);
		}

		template<class tArchiver>
		void archiveRead(tArchiver& ar) {
			Index size;
			ar >> size;
			resize(size);
			ar.readBytes(write(), size);
		}

	public: // Syntax sugars

		explicit StringTemplate(alni val) {
			auto raw = new tChar[MAX_INT_STRING_LENGTH];
			Logic::convertValueToString(val, raw, MAX_INT_STRING_LENGTH);
			mData = new (sStringPool.allocate(0)) StringData(raw);
		}

		explicit StringTemplate(alnf val) {
			auto raw = new tChar[MAX_FLOAT_STRING_LENGTH];
			Logic::convertValueToString(val, raw, MAX_FLOAT_STRING_LENGTH);
			mData = new (sStringPool.allocate(0)) StringData(raw);
		}

		explicit StringTemplate(bool val) {
			auto raw = new tChar[MAX_BOOL_STRING_LENGTH];
			Logic::convertValueToString(val, raw, MAX_BOOL_STRING_LENGTH);
			mData = new (sStringPool.allocate(0)) StringData(raw);
		}

		explicit operator alni() {
			alni out;
			Logic::convertStringToValue(read(), out);
			return out;
		}

		explicit operator alnf() {
			alnf out;
			Logic::convertStringToValue(read(), out);
			return out;
		}

		explicit operator bool() {
			bool out;
			Logic::convertStringToValue(read(), out);
			return out;
		}

		Index size() const {
			return Logic::calcLength(read());
		}

		void calcLineOffsets(Buffer<Index>& aOut) const {
			Logic::calcLineOffsets(read(), size(), aOut);
		}

		StringTemplate& operator=(const tChar* in) {
			this->~StringTemplate();
			new (this) StringTemplate(in);
			return *this;
		}

		StringTemplate& operator=(const StringTemplate& in) {
			if (&in == this) return *this;
			this->~StringTemplate();
			new (this) StringTemplate(in);
			return *this;
		}

		bool operator==(const StringTemplate& in) const {
			if (&in == this) return true;
			if (in.mData == mData) return true;
			return Logic::isEqualLogic(in.read(), read());
		}

	public: // Debugging
		[[nodiscard]] ualni getReferenceCount() const { return mData->mReferenceCount; }
		[[nodiscard]] bool getIsConstFlag() const { return mData->mIsConst; }
	};


	template<typename tChar>
	PoolAlloc<StringData<tChar>, StringTemplate<tChar>::STRINGS_POOL_SIZE> StringTemplate<tChar>::sStringPool;

	template<typename tChar>
	StringData<tChar> StringTemplate<tChar>::sNullString;

	using String = StringTemplate<int1>;

	template<typename tChar>
	ualni hash(StringTemplate<tChar> in) {
		return hash(in.read());
	}
}