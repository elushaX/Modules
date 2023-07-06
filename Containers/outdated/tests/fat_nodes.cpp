
#include "containers.h"

namespace tp {

	struct AvlNode {

		template<typename Type>
		struct EphemeralNode {
			Type mVal = NULL;
			halni mVersion = NULL;
		};

		template<typename Type>
		struct EphemeralsContainer {
			List<EphemeralNode<Type>> mList;
		};

		struct PesistentLink {
			static EphemeralsContainer<AvlNode*> sContainer;
			EphemeralNode<AvlNode*>* mNode = NULL;
			PesistentLink& operator=(const PesistentLink& field) {
			}
			PesistentLink& operator=(AvlNode* val) {
			}
			//Type& operator->() { return mNode->mVal; }
			//const ListIterator& operator*() { return *this; }
			operator AvlNode*() {
			}
		};

		struct PesistentValue {
			static EphemeralsContainer<halni> sContainer;
			EphemeralNode<halni>* mNode = NULL;
			PesistentLink& operator=(const PesistentLink& field) {}
			PesistentLink& operator=(halni val) {}
			operator halni() {
			}
		};

		PesistentLink mLeft;
		PesistentLink mRight;
		PesistentLink mParent;
		PesistentValue mHeight;

		PesistentValue mVal;
		halni mRefCount = 0;

		AvlNode() { mVal = 0; }
		AvlNode(halni aKey) {
			mVal = aKey;
		}

		bool disentRight(halni aKey) {
			return aKey > mVal;
		}

		bool disentLeft(halni aKey) {
			return aKey < mVal;
		}

		bool exactNode(halni aKey) {
			return aKey == mVal;
		}

		halni getFindKey(AvlNode* node = NULL) {
			return mVal;
		}

		void copyData(const AvlNode& in) {
			mVal = in.mVal;
		}

		halni keyInRightSubtree(halni key) { return key; }
		halni keyInLeftSubtree(halni key) { return key; }

		void updateTreeCahceCallBack() {}

		void* operator new(std::size_t) {
		}

		void operator delete(void* p) {
		}
	};
};

void fat_nodes_test() {
	tp::AvlTree<tp::AvlNode, tp::halni> avl;
}