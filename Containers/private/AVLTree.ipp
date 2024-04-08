// #pragma once

// #include "Tree.hpp"

template <typename Key, typename Data, class Allocator>
tp::AvlTree<Key, Data, Allocator>::~AvlTree() {
	removeAll();
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::deleteNode(Node* node) {
	node->~Node();
	mAlloc.deallocate(node);
	mSize--;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::newNode(KeyArg key, DataArg data) -> Node* {
	mSize++;
	auto node = new (mAlloc.allocate(sizeof(Node))) Node(key, data);
	node->mLeft = &gNullNode;
	node->mRight = &gNullNode;
	return node;
}

template <typename Key, typename Data, class Allocator>
[[nodiscard]] tp::ualni tp::AvlTree<Key, Data, Allocator>::size() const {
	return mSize;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::head() const -> Node* {
	return this->mRoot;
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::insert(KeyArg key, DataArg data) {
	if (!mRoot) {
		mRoot = newNode(key, data);
		return;
	}

	if (auto parent = findInsertParent(mRoot, key)) {
		restoreInvariants(insertNode(parent, key, data));
	}
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::remove(KeyArg key) {

	auto node = findSubTree(mRoot, key);

	if (!node) return;

	if (node->mRight != &gNullNode && node->mLeft != &gNullNode) {
		Node* min = minNode(node->mRight);
		injectNodeInstead(node, min);
		std::swap(node, min);
		// auto const& newKey = min->key.getFindKey(head->mRight);
		node = findSubTree(node->mRight, key);
		DEBUG_ASSERT(node);
	}

	if (node->mRight != &gNullNode) {
		if (node->mParent) {
			if (node->mParent->mLeft == node) node->mParent->mLeft = node->mRight;
			else node->mParent->mRight = node->mRight;
		}
		node->mRight->mParent = node->mParent;
		auto delNode = node;
		node = node->mRight;
		deleteNode(delNode);

	} else if (node->mLeft != &gNullNode) {
		if (node->mParent) {
			if (node->mParent->mLeft == node) node->mParent->mLeft = node->mLeft;
			else node->mParent->mRight = node->mLeft;
		}
		node->mLeft->mParent = node->mParent;
		auto delNode = node;
		node = node->mLeft;
		deleteNode(delNode);

	} else {
		if (node->mParent) {
			if (node->mParent->mLeft == node) node->mParent->mLeft = &gNullNode;
			else node->mParent->mRight = &gNullNode;
		}

		auto delNode = node;
		node = node->mParent;
		deleteNode(delNode);
	}

	restoreInvariants(node);

	if (mRoot) mRoot->mParent = nullptr;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::maxNode(Node* head) const -> Node* {
	if (!head) return nullptr;
	while (head->mRight != &gNullNode) {
		head = head->mRight;
	}
	return head;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::minNode(Node* head) const -> Node* {
	if (!head) return nullptr;
	while (head->mLeft != &gNullNode) {
		head = head->mLeft;
	}
	return head;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::find(KeyArg key) const -> Node* {
	return findSubTree(mRoot, key);
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::findSubTree(Node* iter, KeyArg key) const -> Node* {
	while (true) {
		if (iter == &gNullNode) return nullptr;
		if (iter->key.exactNode(key)) return iter;
		if (iter->key.descentRight(key)) {
			// key = iter->key.keyInRightSubtree(key);
			iter = iter->mRight;
		} else {
			// key = iter->key.keyInLeftSubtree(key);
			iter = iter->mLeft;
		}
	}
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::findLessOrEq(KeyArg key) const -> Node* {
	Node* iter = mRoot;
	while (true) {
		if (iter == &gNullNode) return nullptr;
		if (iter->key.exactNode(key)) return iter;
		if (iter->key.descentRight(key)) {
			if (iter->mRight) {
				// key = iter->key.keyInRightSubtree(key);
				iter = iter->mRight;
			} else {
				return iter;
			}
		} else {
			if (iter->mLeft) {
				// key = iter->key.keyInLeftSubtree(key);
				iter = iter->mLeft;
			} else {
				return iter;
			}
		}
	}
}

template <typename Key, typename Data, class Allocator>
bool tp::AvlTree<Key, Data, Allocator>::isValid() {
	return findInvalidNode(head()) == nullptr;
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::removeAll() {
	if (!mRoot) return;
	removeAllUtil(mRoot);
	mRoot = nullptr;
	mSize = 0;
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::removeAllUtil(Node* node) {
	if (node->mLeft != &gNullNode) removeAllUtil(node->mLeft);
	if (node->mRight != &gNullNode) removeAllUtil(node->mRight);
	deleteNode(node);
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::findInvalidNode(const Node* head) const -> const Node* {
	// checks invariants of AVL tree
	// returns first invalid node
	if (!head || head == &gNullNode) return nullptr;

	if (head->mLeft != &gNullNode) {
		// TODO: incomplete test
		if (head->key.descentRight(head->mLeft->key.getFindKey(head))) {
			return head;
		}
		if (head->mLeft->mParent != head) {
			return head;
		}
		if (!head->mRight && head->mLeft->mHeight != head->mHeight - 1) {
			return head;
		}
	}

	if (head->mRight != &gNullNode) {
		if (!head->key.descentRight(head->mRight->key.getFindKey(head))) {
			return head;
		}
		if (head->mRight->mParent != head) {
			return head;
		}
		if (!head->mLeft && head->mRight->mHeight != head->mHeight - 1) {
			return head;
		}
	}

	if (head->mLeft != &gNullNode && head->mRight != &gNullNode) {
		if (max(head->mLeft->mHeight, head->mRight->mHeight) != head->mHeight - 1) {
			return head;
		}
	}

	int balance = head->mRight->mHeight - head->mLeft->mHeight;

	if (balance > 1 || balance < -1) {
		return head;
	}

	const Node* ret = findInvalidNode(head->mRight);

	if (ret) {
		return ret;
	}

	return findInvalidNode(head->mLeft);
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::rotateLeft(Node* pivot) -> Node* {
	// returns new head
	DEBUG_ASSERT(pivot);

	Node* const head = pivot;
	Node* const right = pivot->mRight;
	Node* const right_left = right->mLeft;
	Node* const parent = pivot->mParent;

	// parents
	if (right_left != &gNullNode) right_left->mParent = head;
	head->mParent = right;
	right->mParent = parent;

	// children
	head->mRight = right_left;
	right->mLeft = head;

	// heights
	head->mHeight = 1 + max(head->mLeft->mHeight, head->mRight->mHeight);
	right->mHeight = 1 + max(right->mLeft->mHeight, right->mRight->mHeight);

	// cache
	head->key.updateNodeCache(head);
	right->key.updateNodeCache(right);

	return right;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::rotateRight(Node* pivot) -> Node* {
	// returns new head
	DEBUG_ASSERT(pivot);

	Node* const head = pivot;
	Node* const left = pivot->mLeft;
	Node* const left_right = left->mRight;
	Node* const parent = pivot->mParent;

	// parents
	if (left_right != &gNullNode) left_right->mParent = head;
	head->mParent = left;
	left->mParent = parent;

	// children
	head->mLeft = left_right;
	left->mRight = head;

	// heights
	head->mHeight = 1 + max(head->mLeft->mHeight, head->mRight->mHeight);
	left->mHeight = 1 + max(left->mLeft->mHeight, left->mRight->mHeight);

	// cache
	head->key.updateNodeCache(head);
	left->key.updateNodeCache(left);

	return left;
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::restoreInvariants(Node* head) {
	if (!head) mRoot = nullptr;

	while (head) {
		Node* headParent = head->mParent;
		Node** parentHeadLink = nullptr;

		if (headParent) parentHeadLink = headParent->mLeft == head ? &headParent->mLeft : &headParent->mRight;

		head->mHeight = 1 + max(head->mRight->mHeight, head->mLeft->mHeight);
		const alni balance = head->mRight->mHeight - head->mLeft->mHeight;

		if (balance < -1) {
			if (head->mLeft->mLeft->mHeight >= head->mLeft->mRight->mHeight) {
				head = rotateRight(head);
			} else {
				head->mLeft = rotateLeft(head->mLeft);
				head = rotateRight(head);
			}
		} else if (balance > 1) {
			if (head->mRight->mRight->mHeight >= head->mRight->mLeft->mHeight) {
				head = rotateLeft(head);
			} else {
				head->mRight = rotateRight(head->mRight);
				head = rotateLeft(head);
			}
		}

		head->key.updateNodeCache(head);
		if (headParent) *parentHeadLink = head;

		mRoot = head;
		head = headParent;
	}

	if (mRoot) mRoot->mParent = nullptr;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::insertNode(Node* head, KeyArg key, DataArg data) -> Node* {
	Node* insertedNode = newNode(key, data);

	if (head->key.descentRight(key)) {
		head->mRight = insertedNode;
	} else {
		head->mLeft = insertedNode;
	}

	insertedNode->mParent = head;
	return insertedNode;
}

template <typename Key, typename Data, class Allocator>
auto tp::AvlTree<Key, Data, Allocator>::findInsertParent(Node* head, KeyArg key) -> Node* {
	while (true) {
		if (head->key.exactNode(key)) {
			return nullptr;
		}
		if (head->key.descentRight(key)) {
			if (head->mRight == &gNullNode) {
				return head;
			}
			head = head->mRight;
		} else {
			if (head->mLeft == &gNullNode) {
				return head;
			}
			head = head->mLeft;
		}
	}
}

template <typename Key, typename Data, class Allocator>
void tp::AvlTree<Key, Data, Allocator>::injectNodeInstead(Node* target, Node* from) {
	// swaps pointers of two nodes to preserve data location in the memory instead of swapping data directly

	// 'target' always has two nodes
	// 'from' has only one child on the right on no child at all
	// 'from' can be right child of the 'target'
	// 'target' can or can not have parent
	// 'target' can be left or right child of parent

	// from is not a child of target
	Node* targetParent = target->mParent;
	Node* targetLeft = target->mLeft;
	Node* targetRight = target->mRight;

	bool special = target->mRight == from;

	// update parent
	if (targetParent) {
		if (targetParent->mRight == target) targetParent->mRight = from;
		else targetParent->mLeft = from;
	}

	// clone 'from' to 'target'
	target->mParent = special ? from : from->mParent;
	target->mLeft = &gNullNode;
	target->mRight = from->mRight;
	if (!special) target->mParent->mLeft = target;

	// if (target->mLeft) target->mLeft->mParent = target;
	if (target->mRight) target->mRight->mParent = target;

	// clone 'target' to 'from'
	from->mParent = targetParent;
	from->mLeft = targetLeft;
	from->mRight = special ? target : targetRight;
	from->mLeft->mParent = from;
	from->mRight->mParent = from;

	std::swap(from->mHeight, target->mHeight);
}
