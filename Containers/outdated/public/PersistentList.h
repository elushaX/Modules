#pragma once

#include "List.h"

namespace tp {

	// TODO : insert and remove by range
	// IN DEVELOPMENT
	template <typename Type>
	class PersistentList {

	public:
		struct UndoNode {
			ListNode<Type>* mNode = NULL;
			enum { NONE, ADD, REMOVE, UNDO_MARK } mType = NONE;
		};

	private:

		List<Type> mList;
		List<UndoNode> mHystory;
		ListNode<UndoNode>* mCurrentState = NULL;

		void clearFuture() {
			auto iter = mHystory.last();
			while (iter && iter != mCurrentState) {
				auto del = iter;
				iter = iter->prev;
				mHystory.detach(del);
				mHystory.deleteNode(del);
			}
		}

	public:

		PersistentList() {
			pushUndoPoint();
		}

		void pushUndoPoint() {
			mHystory.pushBack(UndoNode{ 0, UndoNode::UNDO_MARK });
			mCurrentState = mHystory.last();
		}

		const List<Type>& list() const {
			return mList;
		}

		const List<UndoNode>& hystory() const {
			return mHystory;
		}

		// returns new node inserted after aNode
		ListNode<Type>* insertAfter(ListNode<Type>* aNode) {

			clearFuture();

			auto node = mList.newNode();
			mList.attach(node, aNode);
			mHystory.pushBack(UndoNode{ node, UndoNode::ADD });
			return node;
		}

		void remove(ListNode<Type>* aNode) {

			clearFuture();

			mHystory.pushBack(UndoNode{ aNode, UndoNode::REMOVE });
			mList.detach(aNode);
		}

		void undo() {
			if (!mCurrentState->prev) {
				return;
			}

			while (mCurrentState->data.mType != UndoNode::UNDO_MARK) {
				if (mCurrentState->data.mType != UndoNode::ADD) {
					mList.detach(mCurrentState->data.mNode);
				}
				else {
					mList.attach(mCurrentState->data.mNode, mCurrentState->data.mNode->prev);
				}
				mCurrentState = mCurrentState->prev;
			}
		}
		
		void redo() {
			if (!mCurrentState->next) {
				return;
			}

			while (mCurrentState->data.mType != UndoNode::UNDO_MARK) {
				if (mCurrentState->data.mType != UndoNode::ADD) {
					mList.attach(mCurrentState->data.mNode, mCurrentState->data.mNode->prev);
				}
				else {
					mList.detach(mCurrentState->data.mNode);
				}
				mCurrentState = mCurrentState->next;
			}
		}

		void clearHystory() {
			// TODO : mem leaks
			//for (auto node : mHystory) {
				//delete node->data();
			//}
			//mHystory.free();
		}

		~PersistentList() {
		}
	};
};