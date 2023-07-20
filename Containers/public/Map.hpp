
#pragma once

#include "ContainersCommon.hpp"
#include "Common.hpp"

namespace tp {

	template<typename Key>
	ualni DefaultHashFunc(SelCopyArg<Key> key) {
		return hash(key);
	}

	template<typename tKey, typename tVal, class tAllocator = DefaultAllocator, ualni(*tHashFunc)(SelCopyArg<tKey>) = DefaultHashFunc<tKey>, int tTableInitialSize = 4>
	class Map {

		enum {
			MAP_PERTURB_SHIFT = 5,
			MAP_MIN_SIZE = 4,
			MAP_MAX_LOAD_PERCENTAGE = 66,
		};

		typedef SelCopyArg<tKey> KeyArg;
		typedef SelCopyArg<tVal> ValArg;

	public:

		class Node {
			friend Map;
			Node(KeyArg aKey, ValArg aVal) : key(aKey), val(aVal) {}
		public:
			tKey key;
			tVal val;
		};

		struct Idx {
			alni idx = -1;
			operator bool() { return idx != -1; }
		};

	private:
		tAllocator mAlloc;
		Node** mTable;
		ualni mNSlots = 0;
		ualni mNEntries = 0;

	private:

		constexpr halnf maxLoadFactor() { return halnf(MAP_MAX_LOAD_PERCENTAGE) / 100.f; }

		inline Node** newTable(const ualni len) {
			return new(mAlloc.allocate(sizeof(Node*) * len)) Node*[len]();
		}

		inline Node* newNode(KeyArg key, ValArg val) {
			return new(mAlloc.allocate(sizeof(Node))) Node(key, val);
		}

		inline Node* newNodeNotConstructed() {
			return (Node*) mAlloc.allocate(sizeof(Node));
		}

		inline void deleteTable(Node** table) {
			mAlloc.deallocate(table);
		}

		inline void deleteNode(Node* p) {
			p->~Node();
			mAlloc.deallocate(p);
		}

		void markDeletedSlot(ualni idx) const {
			mTable[idx] = (Node*)-1;
		}

		static bool isDeletedNode(Node* node) {
			return node == (Node*)-1;
		}

		void rehash() {
			alni nSlotsOld = mNSlots;
			Node** tableOld = mTable;

			mNSlots = next2pow((uhalni)((1.f / (maxLoadFactor())) * mNEntries + 1));
			mTable = newTable(mNSlots);
			mNEntries = 0;

			for (alni i = 0; i < nSlotsOld; i++) {
				if (!tableOld[i] || isDeletedNode(tableOld[i])) {
					continue;
				}

				alni idx = findSlotWrite(tableOld[i]->key);
				mTable[idx] = tableOld[i];
				mNEntries++;
			}

			deleteTable(tableOld);
		}

		alni findSlotRead(KeyArg key) const {
			ualni const hashed_key = tHashFunc(key);
			ualni const mask = mNSlots - 1;
			ualni const shift = (hashed_key >> MAP_PERTURB_SHIFT) & ~1;
			alni idx = hashed_key & mask;
			NEXT:
			if (isDeletedNode(mTable[idx])) {
				goto SKIP;
			}
			if (!mTable[idx]) {
				return -1;
			}
			if (mTable[idx]->key == key) {
				return idx;
			}
			SKIP:
			idx = ((5 * idx) + 1 + shift) & mask;
			goto NEXT;
		}

		// compares keys only when collisions occur
		alni findSlotReadExisting(KeyArg key) const {
			ualni const hashed_key = tHashFunc(key);
			ualni const mask = mNSlots - 1;
			ualni const shift = (hashed_key >> MAP_PERTURB_SHIFT) & ~1;
			alni idx = hashed_key & mask;
			NEXT:
			if (isDeletedNode(mTable[idx])) {
				goto SKIP;
			}
			if (!mTable[idx]) {
				return -1;
			}
			if (mTable[((5 * idx) + 1 + shift) & mask] == nullptr) {
				return idx;
			}
			if (mTable[idx]->key == key) {
				return idx;
			}
			SKIP:
			idx = ((5 * idx) + 1 + shift) & mask;
			goto NEXT;
		}

		ualni findSlotWrite(KeyArg key) const {
			ualni const hashed_key = tHashFunc(key);
			ualni const mask = mNSlots - 1;
			ualni const shift = (hashed_key >> MAP_PERTURB_SHIFT) & ~1;
			ualni idx = hashed_key & mask;
			NEXT:
			if (isDeletedNode(mTable[idx]) || !mTable[idx]) {
				return idx;
			}
			if (mTable[idx]->key == key) {
				return idx;
			}
			idx = ((5 * idx) + 1 + shift) & mask;
			goto NEXT;
		}

		void put(Node* node) {
			const ualni idx = findSlotWrite(node->key);

			if (!mTable[idx] || isDeletedNode(mTable[idx])) {
				mNEntries++;
			}

			mTable[idx] = node;

			if ((halnf)mNEntries / mNSlots > maxLoadFactor()) {
				rehash();
			}
		}

	public:

		Map() {
			MODULE_SANITY_CHECK(gModuleContainers)
			mNSlots = next2pow(uhalni(tTableInitialSize - 1));
			mTable = newTable(mNSlots);
		}

		Node** buff() const {
			return mTable;
		}

		[[nodiscard]] ualni size() const {
			return mNEntries;
		}

		[[nodiscard]] ualni slotsSize() const {
			return mNEntries;
		}

		[[nodiscard]] const tAllocator& getAllocator() const {
			return mAlloc;
		}

		void put(KeyArg key, ValArg val) {
			const ualni idx = findSlotWrite(key);
			if (!mTable[idx] || isDeletedNode(mTable[idx])) {
				mTable[idx] = newNode(key, val);
				mNEntries++;
			}
			mTable[idx]->val = val;
			if ((halnf) mNEntries / mNSlots > maxLoadFactor()) {
				rehash();
			}
		}

		// undefined behavior if item is not presents
		tVal& get(KeyArg key) {
			DEBUG_ASSERT(findSlotRead(key) != -1 && "Key Error")
			return mTable[findSlotReadExisting(key)]->val;
		}

		const tVal& get(KeyArg key) const {
			DEBUG_ASSERT(findSlotRead(key) != -1 && "Key Error")
			return mTable[findSlotReadExisting(key)]->val;
		}

		[[nodiscard]] Idx presents(KeyArg key) const { return { findSlotRead(key) }; }

		void remove(KeyArg key) {
			DEBUG_ASSERT(findSlotRead(key) != -1 && "Key Error")
			auto idx = findSlotReadExisting(key);

			deleteNode(mTable[idx]);

			markDeletedSlot(idx);

			mNEntries--;
			if (halnf(mNEntries / mNSlots) < 1.f - maxLoadFactor()) {
				rehash();
			}
		}

		const tVal& getSlotVal(ualni slot) const {
			DEBUG_ASSERT(slot < mNSlots && (mTable[slot] && !isDeletedNode(mTable[slot])) && "Key Error")
			return mTable[slot]->val;
		}
		
		tVal& getSlotVal(ualni slot) {
			DEBUG_ASSERT(slot < mNSlots && (mTable[slot] && !isDeletedNode(mTable[slot])) && "Key Error")
			return mTable[slot]->val;
		}

		const tVal& getSlotVal(Idx slot) const {
			DEBUG_ASSERT(slot.idx < mNSlots && (mTable[slot.idx] && !isDeletedNode(mTable[slot.idx])) && "Key Error")
			return mTable[slot.idx]->val;
		}

		tVal& getSlotVal(Idx slot) {
			DEBUG_ASSERT(slot.idx < mNSlots && (mTable[slot.idx] && !isDeletedNode(mTable[slot.idx])) && "Key Error")
			return mTable[slot.idx]->val;
		}

		Map& operator=(const Map& in) {
			if (this == &in) {
				return *this;
			}
			removeAll();
			mNSlots = in.mNSlots;
			mTable = newTable(mNSlots);
			for (alni i = 0; i < mNSlots; i++) {
				if (in.mTable[i] && !isDeletedNode(in.mTable[i])) {
					put(in.mTable[i]->key, in.mTable[i]->val);
				}
			}
			return *this;
		}

		[[nodiscard]] bool operator==(const Map& in) const {
			if (this == &in) {
				return true;
			}
			if (in.mNEntries != mNEntries) {
				return false;
			}
			for (auto i : in) {
				if (!presents(i->key) || get(i->key) != i->val) {
					return false;
				}
			}
			return true;
		}

		void removeAll() {
			for (ualni i = 0; i < mNSlots; i++) {
				if (mTable[i] && !isDeletedNode(mTable[i])) {
					deleteNode(mTable[i]);
				}
			}
			deleteTable(mTable);
			mTable = newTable(tTableInitialSize);
			mNSlots = tTableInitialSize;
			mNEntries = 0;
		}

		[[nodiscard]] alni slotIdx(alni entry_idx_in) const {
			alni entry_idx = -1;
			for (alni slot_idx = 0; slot_idx < mNSlots; slot_idx++) {
				if (mTable[slot_idx]) {
					entry_idx++;
				}
				if (entry_idx == entry_idx_in) {
					return slot_idx;
				}
			}
			return -1;
		}

		Node* GetEntry(ualni idx) { 
			auto slot = slotIdx(idx);
			DEBUG_ASSERT(slot != -1 && "Key error")
			return mTable[slot];	
		}

		const Node* GetEntry(ualni idx) const { 
			auto slot = slotIdx(idx);
			DEBUG_ASSERT(slot != -1 && "Key error")
			return mTable[slot];
		}

	public:

		class Iterator {
			const Map* map;
			Node* mIter;
			alni mSlot;
			alni mEntry;

			friend Map;
			explicit Iterator(const Map* _map) {
				mSlot = -1;
				mEntry = -1;
				map = _map;
				this->operator++();
			}

		public:
			Node* operator->() { return mIter; }
			const Node* operator->() const { return mIter; }
			const Iterator& operator*() const { return *this; }

			bool operator!=(ualni idx) const { return mSlot != idx; }

			void operator++() {
				mSlot++;
				while ((map->isDeletedNode(map->mTable[mSlot]) || !map->mTable[mSlot]) && (mSlot != map->mNSlots)) {
					mSlot++;
				}
				if (mSlot != map->mNSlots) {
					mIter = map->mTable[mSlot];
					mEntry++;
				}
			}
		};

		[[nodiscard]] Iterator begin() const {
			return Iterator(this);
		}

		[[nodiscard]] ualni end() const {
			return mNSlots;
		}

		template<class Saver>
		void write(Saver& file) {
			file.write(mNEntries);
			for (auto item : *this) {
				file.write(item->val);
				file.write(item->key);
			}
		}

		template<class Loader>
		void read(Loader& file) {
			removeAll();
			ualni len;
			file.read(len);
			for (auto i = len; i; i--) {
				auto node = newNodeNotConstructed();
				file.read(node->val);
				file.read(node->key);
				put(node);
			}
		}

		~Map() {
			for (ualni i = 0; i < mNSlots; i++) {
				if (mTable[i] && !isDeletedNode(mTable[i])) {
					deleteNode(mTable[i]);
				}
			}
			deleteTable(mTable);
		}
	};
}