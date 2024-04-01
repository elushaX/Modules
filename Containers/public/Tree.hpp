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

		private:
			Node(KeyArg aKey, DataArg aData) :
				key(aKey),
				data(aData) {}

		public:
			Data data;
			Key key;

			Node* mLeft = nullptr;
			Node* mRight = nullptr;
			Node* mParent = nullptr;
			ualni mHeight = 0;
		};

	public:
		AvlTree() {}
		~AvlTree() { removeAll(); }

		[[nodiscard]] ualni size() const { return mSize; }

		Node* head() const { return this->mRoot; }

		void insert(KeyArg key, DataArg data) {
			mRoot = insertUtil(mRoot, key, data);
			mRoot->mParent = nullptr;
		}

		void remove(KeyArg key) {
			mRoot = removeUtil(mRoot, key);
			if (mRoot) mRoot->mParent = nullptr;
		}

		Node* maxNode(Node* head) const {
			if (!head) return nullptr;
			while (head->mRight != nullptr) {
				head = head->mRight;
			}
			return head;
		}

		Node* minNode(Node* head) const {
			if (!head) return nullptr;
			while (head->mLeft != nullptr) {
				head = head->mLeft;
			}
			return head;
		}

		Node* find(KeyArg key) const {
			Node* iter = mRoot;
			while (true) {
				if (!iter) return nullptr;
				if (iter->key.exactNode(key)) return iter;
				if (iter->key.descentRight(key)) {
					key = iter->key.keyInRightSubtree(key);
					iter = iter->mRight;
				} else {
					key = iter->key.keyInLeftSubtree(key);
					iter = iter->mLeft;
				}
			}
		}

		Node* findLessOrEq(KeyArg key) const {
			Node* iter = mRoot;
			while (true) {
				if (!iter) return nullptr;
				if (iter->key.exactNode(key)) return iter;
				if (iter->key.descentRight(key)) {
					if (iter->mRight) {
						key = iter->key.keyInRightSubtree(key);
						iter = iter->mRight;
					} else {
						return iter;
					}
				} else {
					if (iter->mLeft) {
						key = iter->key.keyInLeftSubtree(key);
						iter = iter->mLeft;
					} else {
						return iter;
					}
				}
			}
		}

		// returns first invalid node
		const Node* findInvalidNode(const Node* head) const {
			if (head == nullptr) return nullptr;

			if (head->mLeft) {
				// TODO: incomplete test
				if (head->key.descentRight(head->mLeft->key.getFindKey(head))) return head;
				if (head->mLeft->mParent != head) return head;
				if (!head->mRight && head->mLeft->mHeight != head->mHeight - 1) return head;
			}

			if (head->mRight) {
				if (!head->key.descentRight(head->mRight->key.getFindKey(head))) return head;
				if (head->mRight->mParent != head) return head;
				if (!head->mLeft && head->mRight->mHeight != head->mHeight - 1) return head;
			}

			if (head->mLeft && head->mRight) {
				if (max(head->mLeft->mHeight, head->mRight->mHeight) != head->mHeight - 1) return head;
			}

			int balance = getNodeHeight(head->mRight) - getNodeHeight(head->mLeft);

			if (balance > 1 || balance < -1) return head;

			const Node* ret = findInvalidNode(head->mRight);

			if (ret) return ret;

			return findInvalidNode(head->mLeft);
		}

		bool isValid() { return findInvalidNode(head()) == nullptr; }

		template <typename tFunctor>
		void traverse(Node* node, bool after, tFunctor functor) {
			if (!after) functor(node);
			if (node->mLeft) traverse(node->mLeft, after, functor);
			if (node->mRight) traverse(node->mRight, after, functor);
			if (after) functor(node);
		}

		void removeAll() {
			if (!mRoot) return;
			removeUtil(mRoot);
			mRoot = nullptr;
			mSize = 0;
		}

		void removeUtil(Node* node) {
			if (node->mLeft) removeUtil(node->mLeft);
			if (node->mRight) removeUtil(node->mRight);
			deleteNode(node);
		}

	public:
		template <class tArchiver>
		void archiveWrite(tArchiver& file) const {
			FAIL("not implemented")
		}

		template <class tArchiver>
		void archiveRead(tArchiver&) {
			FAIL("not implemented")
		}

	private:
		inline void deleteNode(Node* node) {
			node->~Node();
			mAlloc.deallocate(node);
		}

		inline Node* newNode(KeyArg key, DataArg data) { return new (mAlloc.allocate(sizeof(Node))) Node(key, data); }

		inline void injectNodeInstead(Node* place, Node* inject) {
			// TODO : swap instead of copy
			place->data = inject->data;
			place->key = inject->key;
		}

		inline alni getNodeHeight(const Node* node) const { return node ? node->mHeight : -1; }

		// returns new head
		Node* rotateLeft(Node* pivot) {
			DEBUG_ASSERT(pivot);

			Node* const head = pivot;
			Node* const right = pivot->mRight;
			Node* const right_left = right->mLeft;
			Node* const parent = pivot->mParent;

			// parents
			if (right_left) right_left->mParent = head;
			head->mParent = right;
			right->mParent = parent;

			// children
			head->mRight = right_left;
			right->mLeft = head;

			// heights
			head->mHeight = 1 + max(getNodeHeight(head->mLeft), getNodeHeight(head->mRight));
			right->mHeight = 1 + max(getNodeHeight(right->mLeft), getNodeHeight(right->mRight));

			// cache
			head->key.updateNodeCache(head);
			right->key.updateNodeCache(right);

			return right;
		}

		Node* rotateRight(Node* pivot) {
			DEBUG_ASSERT(pivot);

			Node* const head = pivot;
			Node* const left = pivot->mLeft;
			Node* const left_right = left->mRight;
			Node* const parent = pivot->mParent;

			// parents
			if (left_right) left_right->mParent = head;
			head->mParent = left;
			left->mParent = parent;

			// children
			head->mLeft = left_right;
			left->mRight = head;

			// heights
			head->mHeight = 1 + max(getNodeHeight(head->mLeft), getNodeHeight(head->mRight));
			left->mHeight = 1 + max(getNodeHeight(left->mLeft), getNodeHeight(left->mRight));

			// cache
			head->key.updateNodeCache(head);
			left->key.updateNodeCache(left);

			return left;
		}

		// recursively returns valid isLeft or isRight child or root
		Node* insertUtil(Node* head, KeyArg key, DataArg data) {

			Node* insertedNode;

			if (head == nullptr) {
				mSize++;
				Node* out = newNode(key, data);
				out->key.updateNodeCache(out);
				return out;
			} else if (head->key.exactNode(key)) {
				return head;
			} else if (head->key.descentRight(key)) {
				insertedNode = insertUtil(head->mRight, head->key.keyInRightSubtree(key), data);
				head->mRight = insertedNode;
				insertedNode->mParent = head;
			} else {
				insertedNode = insertUtil(head->mLeft, head->key.keyInLeftSubtree(key), data);
				head->mLeft = insertedNode;
				insertedNode->mParent = head;
			}

			// update height
			head->mHeight = 1 + max(getNodeHeight(head->mRight), getNodeHeight(head->mLeft));

			alni balance = alni(getNodeHeight(head->mRight) - getNodeHeight(head->mLeft));

			if (balance > 1) {
				if (head->mRight->key.descentRight(head->key.keyInRightSubtree(key))) {
					return rotateLeft(head);
				} else {
					head->mRight = rotateRight(head->mRight);
					return rotateLeft(head);
				}
			} else if (balance < -1) {
				if (!head->mLeft->key.descentRight(head->key.keyInLeftSubtree(key))) {
					return rotateRight(head);
				} else {
					head->mLeft = rotateLeft(head->mLeft);
					return rotateRight(head);
				}
			}

			head->key.updateNodeCache(head);

			return head;
		}

		Node* removeUtil(Node* head, KeyArg key) {
			if (head == nullptr) return head;

			if (head->key.exactNode(key)) {
				if (head->mRight && head->mLeft) {
					Node* min = minNode(head->mRight);
					auto const& newKey = min->key.getFindKey(head->mRight);
					injectNodeInstead(head, min);
					head->mRight = removeUtil(head->mRight, newKey);
				} else if (head->mRight) {
					injectNodeInstead(head, head->mRight);
					deleteNode(head->mRight);
					head->mRight = nullptr;
					mSize--;
				} else if (head->mLeft) {
					injectNodeInstead(head, head->mLeft);
					deleteNode(head->mLeft);
					head->mLeft = nullptr;
					mSize--;
				} else {
					deleteNode(head);
					mSize--;
					head = nullptr;
				}
			} else if (head->key.descentRight(key)) {
				head->mRight = removeUtil(head->mRight, head->key.keyInRightSubtree(key));
			} else {
				head->mLeft = removeUtil(head->mLeft, head->key.keyInLeftSubtree(key));
			}

			if (head == nullptr) return head;

			head->mHeight = 1 + max(getNodeHeight(head->mRight), getNodeHeight(head->mLeft));
			alni balance = getNodeHeight(head->mRight) - getNodeHeight(head->mLeft);

			if (balance < -1) {
				if (getNodeHeight(head->mLeft->mLeft) >= getNodeHeight(head->mLeft->mRight)) {
					return rotateRight(head);
				} else {
					head->mLeft = rotateLeft(head->mLeft);
					return rotateRight(head);
				}
			} else if (balance > 1) {
				if (getNodeHeight(head->mRight->mRight) >= getNodeHeight(head->mRight->mLeft)) {
					return rotateLeft(head);
				} else {
					head->mRight = rotateRight(head->mRight);
					return rotateLeft(head);
				}
			}

			head->key.updateNodeCache(head);

			return head;
		}

	private:
		Node* mRoot = nullptr;
		ualni mSize = 0;
		Allocator mAlloc;
	};
}