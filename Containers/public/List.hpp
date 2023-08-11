
#pragma once

#include "ContainersCommon.hpp"

#include "TypeInfo.hpp"

namespace tp {

	template <typename Type, class Allocator = DefaultAllocator>
	class List {

		typedef SelectValueOrReference<Type> TypeArg;
		typedef ualni Index;

	public:

		struct Node {
			Type data;
			Node* next = nullptr;
			Node* prev = nullptr;
			Node() = default;
			explicit Node(TypeArg p_data) : data(p_data) {}
			Type& operator->() { return data; }
		};

		class IteratorPointer {
		protected:
			Node* mIter;
		public:
			IteratorPointer() = default;
			Type& operator->() { return (mIter->data); }
			const Type& operator->() const { return (mIter->data); }
		};

		class IteratorReference {
		protected:
			Node* mIter;
		public:
			IteratorReference() = default;
			Type* operator->() { return &(mIter->data); }
			const Type* operator->() const { return &(mIter->data); }
		};

		class Iterator : public TypeSelect<TypeTraits<Type>::isPointer, IteratorPointer, IteratorReference>::Result {
		public:

			explicit Iterator(Node* iter) { this->mIter = iter; }

			Node* node() { return this->mIter; }
			Type& data() { return this->mIter->data; }
			const Node* node() const { return this->mIter; }
			const Type& data() const { return this->mIter->data; }

			const Iterator& operator*() const { return *this; }

			Iterator& operator++() {
				this->mIter = this->mIter->next;
				return *this;
			}

			bool operator==(const Iterator& left) const { return left.mIter == this->mIter; }
			bool operator!=(const Iterator& left) const { return left.mIter != this->mIter; }
		};

	private:
		Node* mFirst = nullptr;
		Node* mLast = nullptr;
		Index mLength = 0;
		Allocator mAlloc;

	public:

		List() = default;
		List(const List& in) { this->operator=(in);	}
		List(const InitialierList<Type>& list) { operator=(list); }

		[[nodiscard]] inline Node* first() const { return mFirst; }
		[[nodiscard]] inline Node* last() const { return mLast; }
		[[nodiscard]] inline Index length() const { return mLength; }

		[[nodiscard]] Node* newNode() { return new (mAlloc.allocate(sizeof(Node))) Node(); }
		[[nodiscard]] Node* newNode(TypeArg arg) { return new (mAlloc.allocate(sizeof(Node))) Node(arg); }
		[[nodiscard]] Node* newNodeNotConstructed() {
			auto node = (Node*) mAlloc.allocate(sizeof(Node));
			node->next = node->prev = nullptr;
			return node;
		}

		[[nodiscard]] const Allocator& getAllocator() const { return mAlloc; }

		void deleteNode(Node* node) {
			node->~Node();
			mAlloc.deallocate(node);
		}

		Node* addNodeBack() {
			auto const out = newNode();
			pushBack(out);
			return out;
		}

		Node* addNodeFront() {
			auto const out = newNode();
			pushFront(out);
			return out;
		}

		void attach(Node* node, Node* node_to) {
			if (node_to) {
				if (node_to->next) {
					node->next = node_to->next;
					node->next->prev = node;
				}
				node_to->next = node;
				node->prev = node_to;
				if (node_to == mLast) {
					mLast = node;
				}
			} else {
				if (mFirst) {
					mFirst->prev = node;
					node->next = mFirst;
					mFirst = node;
				} else {
					mFirst = mLast = node;
				}
			}
			mLength++;
		}

		void detach(Node* node) {
			if (node->next) {
				node->next->prev = node->prev;
			}
			if (node->prev) {
				node->prev->next = node->next;
			}
			if (node == mLast) {
				mLast = mLast->prev;
			}
			if (node == mFirst) {
				mFirst = mFirst->next;
			}
			mLength--;
		}

		[[nodiscard]] Node* findIdx(Index idx) const {
			DEBUG_ASSERT(mFirst && idx < mLength && idx >= 0)
			Node* found = mFirst;
			for (int i = 0; i != idx; i++) {
				found = found->next;
			}
			return found;
		}

		[[nodiscard]] Node* find(const TypeArg data) const {
			Node* found = mFirst;
			for (alni i = 0; data != found->data; i++) {
				if (!found->next) {
					return nullptr;
				}
				found = found->next;
			}
			return found;
		}

		[[nodiscard]] inline const Type& operator[](Index idx) const {
			DEBUG_ASSERT(idx < mLength)
			return find(idx)->data;
		}

		void pushBack(Node* new_node) { attach(new_node, mLast); }
		void pushFront(Node* new_node) { attach(new_node, nullptr); }
		void pushBack(TypeArg data) { pushBack(newNode(data)); }
		void pushFront(TypeArg data) { pushFront(newNode(data)); }

		void popBack() {
			DEBUG_ASSERT(mLast)
			auto const tmp = mLast;
			detach(mLast);
			deleteNode(tmp);
		}

		void popFront() {
			DEBUG_ASSERT(mFirst)
			auto temp = mFirst;
			detach(mFirst);
			deleteNode(temp);
		}

		void insert(Node* node, Index idx) {
			if (!mLength) {
				attach(node, mLast);
			} else if (idx >= mLength) {
				attach(node, nullptr);
			} else {
				attach(node, find(idx)->prev);
			}
		}

		void insert(TypeArg data, Index idx) {
			insert(newNode(data), idx);
		}

		void removeNode(Node* node) {
			detach(node);
			deleteNode(node);
		}

		void removeAll() {
			while (mFirst) {
				popFront();
			}
		}

		// copies data
		List& operator+=(const List& in) {
			for (auto node : in) {
				pushBack(node.data());
			}
			return *this;
		}

		List& operator+=(const InitialierList<Type>& list) {
			for (auto item : list) {
				pushBack(item);
			}
			return *this;
		}

		List& operator=(const List& in) {
			if (this == &in) { return *this; }
			removeAll();
			(*this) += in;
			return *this;
		}

		List& operator=(const InitialierList<Type>& list) {
			removeAll();
			*this += list;
			return *this;
		}

		[[nodiscard]] bool operator==(const List& in) const {
			if (&in == this) { return true; }
			if (in.length() != length()) {
				return false;
			}
			Node* left = in.first();
			Node* right = first();
			while (left && right) {
				if (left->data != right->data) {
					return false;
				}
				left = left->next;
				right = right->next;
			}
			if (left != right) {
				return false;
			}
			return true;
		}

		template <typename compare_val>
		[[nodiscard]] Node* find(bool (*found)(Node* node, compare_val val), compare_val value) const {
			for (Node* node = mFirst; node; node = node->next) {
				if (found(node, value)) {
					return node;
				}
			}
			return nullptr;
		}

		[[nodiscard]] Iterator begin() const {
			Iterator out(mFirst);
			return out;
		}

		[[nodiscard]] Iterator end() const {
			return Iterator(nullptr);
		}

		void invert() {
			Node* iter = mFirst;
			Node* tmp;
			while (iter) {
				tmp = iter;
				iter = iter->next;
				swap(tmp->next, tmp->prev);
			}
			swap(mFirst, mLast);
		}

		void detachAll() {
			while (mFirst) {
				detach(mFirst);
			}
		}

	public:
		template<class tArchiver>
		void archiveWrite(tArchiver& ar) const {
			ar << mLength;
			for (auto item : *this) {
				ar << item.data();
			}
		}

		template<class tArchiver>
		void archiveRead(tArchiver& ar) {
			removeAll();
			decltype(mLength) len;
			ar >> len;
			for (auto i = len; i; i--) {
				auto node = newNodeNotConstructed();
				ar >> node->data;
				pushBack(node);
			}
		}

		~List() {
			removeAll();
		}
	};
}