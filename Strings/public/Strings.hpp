#pragma once

#include "PoolAllocator.hpp"
#include "StringLogic.hpp"
#include "TextEditor.hpp"

namespace tp {

	// Static data
	extern ModuleManifest gModuleStrings;

	template<typename tChar>
	class StringTemplate;
	using String = StringTemplate<uint1>;

	template<typename tChar = uint1>
	class StringTemplate {
		typedef StringLogic<tChar> StringLogic;

		// Hidden slave class
		class StringData {

			// allows to edit each string with text editor inplace
			class TextEditors {
				enum { MAX_EDITED_STRINGS = 65536 }; // 2^16 - do not change mindlessly
				typedef uint2 Address;
				ChunkAlloc<TextEditor*, MAX_EDITED_STRINGS> mEditorsPool;

			public:
				TextEditors() : mEditorsPool() {}

				TextEditor* get(Address address) { return mEditorsPool.getBuff() + address;	}

				Address createTextEditor(const tChar* originalBuffer) {
					auto entry = (TextEditor**) mEditorsPool.allocate(0);
					*entry = new TextEditor( originalBuffer );
					return Address(entry - mEditorsPool.getBuff());
				}

				void destroyTextEditor(Address address) {
					auto entry = mEditorsPool.getBuff() + address;
					mEditorsPool.deallocate(entry);
					delete entry;
				}
			};

			static TextEditors sTextEditors;

		public:
			uint2 mIsConst; // source is non-modifiable
			uint2 mEditedIdx; // editor id
			uint4 mReferenceCount; // number of users of this
			tChar* mBuff; // actual string data

		public:
			StringData() {
				MODULE_SANITY_CHECK(gModuleStrings)
				mBuff = (tChar*) "*";
				mReferenceCount = 0;
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
				auto const len = StringLogic::calcLength(aBuff);
				resize(len);
				memCopy(mBuff, aBuff, len);
				mReferenceCount = 0;
				mIsConst = false;
				mEditedIdx = 0;
			}

			[[nodiscard]] tChar* getBuffer() {
				DEBUG_ASSERT(!isEditorMode())
				if (isEditorMode()) return nullptr;
				return mBuff;
			}

			~StringData() {
				if (!mIsConst) delete[] mBuff;
				if (isEditorMode()) sTextEditors.destroyTextEditor(mEditedIdx);
			}

		public:
			void resize(ualni size) {
				DEBUG_ASSERT(!isEditorMode() && mReferenceCount == 1 && !mIsConst)
				delete[] mBuff;
				mBuff = new tChar[size + 1];
				mBuff[size] = StringLogic::getEndChar();
			}

		public:
			[[nodiscard]] bool isEditorMode() const { return mEditedIdx; }

			void createEditor() {
				DEBUG_ASSERT(!isEditorMode())
				if (isEditorMode()) mEditedIdx = sTextEditors.createTextEditor(mBuff);
			}

			[[nodiscard]] TextEditor* getEditor() const {
				DEBUG_ASSERT(isEditorMode())
				if (!isEditorMode()) return nullptr;
				return sTextEditors.get(mEditedIdx);
			}

			void applyEditor() {
				DEBUG_ASSERT(isEditorMode() && mReferenceCount == 1 && !mIsConst)
				if (!isEditorMode()) return;
				auto editor = sTextEditors.get(mEditedIdx);
				resize(editor.textSize());
				for ( auto character : *editor) {
					mBuff[character.idx] = character;
				}
			}

			void destroyEditor() {
				DEBUG_ASSERT(isEditorMode())
				if (!isEditorMode()) return;
				sTextEditors.destroyTextEditor(mEditedIdx);
			}
		};

	private:
		class StringData* mStringData;

	private:
		enum { STRINGS_POOL_SIZE = 1024, };
		static PoolAlloc<StringData, STRINGS_POOL_SIZE>* sStringPool;
		static StringData sNullString;

		//  --------------------------------- Main Interface ---------------------------------//
	public:
		// Creates null string reference
		StringTemplate() {
			mStringData = &sNullString;
			incReference(mStringData);
		}

		// References existing string data
		StringTemplate(const StringTemplate& in) {
			if (in.isEditorMode()) {
				// Copies raw data and claims ownership over it
				mStringData = new (sStringPool->allocate(0)) StringData(in.mStringData->mBuff);
				incReference(mStringData);
			} else {
				mStringData = in.mStringData;
				incReference(mStringData);
			}
		}

		// Creates new string data from raw data pointer.
		// Does not own string buffer - string buffer will not be freed. Initializes as const memory.
		explicit StringTemplate(const tChar* raw) {
			mStringData = new (sStringPool->allocate(0)) StringData(raw, true);
			incReference(mStringData);
		}

		// Copies raw data and claims ownership over it
		explicit StringTemplate(tChar* raw) {
			mStringData = new (sStringPool->allocate(0)) StringData(raw);
			incReference(mStringData);
		}

		~StringTemplate() {
			decReference(mStringData);
		}

	private:

		[[nodiscard]] bool isEditorMode() const {
			return mStringData->isEditorMode();
		}

		static void incReference(StringData* dp) {
			dp->mReferenceCount++;
		}

		void decReference(StringData* dp) {
			dp->mReferenceCount--;
			if (!dp->mReferenceCount) {
				sStringPool->deallocate(mStringData);
				mStringData->~StringData();
			}
		}

	public: // Access
		// used to read data
		[[nodiscard]] const tChar* read() const {
			return mStringData->mBuff;
		}

		// used to write data
		// output will be null if in TextEditing mode
		[[nodiscard]] tChar* write() {
			makeModifiable();
			return mStringData->getBuffer();
		}

		// output will be null if in TextEditing mode
		[[nodiscard]] tChar* resize() {
			if (!mStringData->getEditor()) mStringData->resize();
			return mStringData->getBuffer();
		}

	public: // TextEditor Mode
		// Enters TextEditor Mode
		void createEditor() { mStringData->createEdited(); }

		// Access to TextEditor Context & Interface
		[[nodiscard]] TextEditor* getEditor() { return mStringData->getEdited(); }
		[[nodiscard]] const TextEditor* getEditor() const { return mStringData->getEdited(); }

		// Applies TextEditor Mode To Current buffer
		void applyEditor() { mStringData->applyEditor(); }

		// Leaves TextEditor Mode
		void destroyEditor() { mStringData->destroyEditor(); }

	private:
		void makeModifiable() {
			// We have no rights to modify if someone references that memory too
			// So we need to create new copy of StringData
			if (mStringData->mReferenceCount > 1) {
				mStringData = new (sStringPool->allocate(0)) StringData(mStringData->mBuff);
				incReference(mStringData);
				return;
			}

			// Also if initial raw data was marked as const - create copy of raw data
			// Note - raw data will be lost at this point if no other record of such is stored
			if (mStringData->mIsConst) {
				mStringData = new (sStringPool->allocate(0)) StringData(mStringData->mBuff);
				incReference(mStringData);
			}
		}
	};
}