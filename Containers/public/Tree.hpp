#pragma once

#include "ContainersCommon.hpp"

namespace tp {

	template <typename NumericType>
	struct AvlNumericKey {
	public:
		AvlNumericKey() = default;
		AvlNumericKey(NumericType val) :
			val(val) {}

		inline bool descentRight(const AvlNumericKey& in) const { return in.val > val; }
		inline bool exactNode(const AvlNumericKey& in) const { return in.val == val; }

		inline const AvlNumericKey& keyInRightSubtree(const AvlNumericKey& in) const { return in; }
		inline const AvlNumericKey& keyInLeftSubtree(const AvlNumericKey& in) const { return in; }

		template <typename NodeType>
		inline void updateNodeCache(const NodeType*) {}

		template <typename NodeType>
		inline const AvlNumericKey& getFindKey(const NodeType*) const {
			return *this;
		}

	public:
		NumericType val;
	};

	template <typename Key, typename Data, class Allocator = DefaultAllocator>
	class AvlTree {
		typedef SelectValueOrReference<Key> KeyArg;
		typedef SelectValueOrReference<Data> DataArg;

	public:
		class Node {
			friend AvlTree;

		public:
			Node() = default;

		private:
			Node(KeyArg aKey, DataArg aData) :
				key(aKey),
				data(aData) {}

		public:
			alni mHeight = -1;

			Node* mLeft = nullptr;
			Node* mRight = nullptr;
			Node* mParent = nullptr;

			Data data;
			Key key;
		};

	public:
		AvlTree() = default;
		~AvlTree();

		ualni size() const;
		Node* head() const;

		void insert(KeyArg key, DataArg data);
		void remove(KeyArg key);
		void removeAll();

		Node* maxNode(Node* head) const;
		Node* minNode(Node* head) const;

		Node* find(KeyArg key) const;
		Node* findSubTree(Node* iter, KeyArg key) const;
		Node* findLessOrEq(KeyArg key) const;

		template <typename tFunctor>
		void traverse(Node* node, bool after, tFunctor functor) {
			if (!after) functor(node);
			if (node->mLeft) traverse(node->mLeft, after, functor);
			if (node->mRight) traverse(node->mRight, after, functor);
			if (after) functor(node);
		}

		auto findInvalidNode(const Node* head) const -> const Node*;
		bool isValid();

	private:
		inline void deleteNode(Node* node);
		inline auto newNode(KeyArg key, DataArg data) -> Node*;

		inline auto rotateLeft(Node* pivot) -> Node*;
		inline auto rotateRight(Node* pivot) -> Node*;

		inline void restoreInvariants(Node* head);

		inline auto insertNode(Node* head, KeyArg key, DataArg data) -> Node*;
		inline auto findInsertParent(Node* head, KeyArg key) -> Node*;

		inline void injectNodeInstead(Node* target, Node* from);

		void removeAllUtil(Node* node);

	private:
		Node* mRoot = nullptr;
		ualni mSize = 0;
		Allocator mAlloc;

		static Node gNullNode;
	};

	template <typename Key, typename Data, class Allocator>
	AvlTree<Key, Data, Allocator>::Node AvlTree<Key, Data, Allocator>::gNullNode;
}

#include "AVLTree.ipp"
