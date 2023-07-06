#pragma once

#include "allocators.h"

namespace tp {
	extern ModuleManifest gModuleContainer;

	template <typename Type, typename Allocator = tp::PickAlloc>
	class Stack {
	public:

		template <typename NodeType>
		class Node {
		public:
			Node<NodeType>* below;
			NodeType data;

		public:
			Node(NodeType data, Node<NodeType>* below) {
				this->below = below;
				this->data = data;
			}
		};

		template <typename IterType>
		struct Iterator {
			Node<IterType>* mIter;
			IterType& operator->() { return mIter->data; }
			const Iterator& operator*() { return *this; }
			inline void operator++() { mIter = mIter->below; }
			bool operator==(alni end) { return mIter == NULL; }
		};

		alni length;
		Node<Type>* top;

		Allocator alloc;

		Stack() {
			MODULE_SANITY_CHECK(gModuleContainer);
			length = 0;
			top = nullptr;
		}

		~Stack() {
			free();
		}

		// pushes the node on top of the stack
		void push(Type data) {
			Node<Type>* NewNode = new (alloc) Node<Type>(data, top);
			top = NewNode;
			length++;
		}

		// pops the top node of the stack
		void pop() {
			assert(top);
			Node<Type>* del = top;
			top = top->below;
			delete del;
			length--;
		}

		Node<Type>* last() {
			return top;
		}

		// deletes all nodes
		void free() {
			Node<Type>* del = top;
			for (alni i = 0; i < length; i++) {
				Node<Type>* below = del->below;
				delete del;
				del = below;
			}
			length = 0;
			top = NULL;
		}

		alni sizeAllocatedMem() {
			alni out = 0;
			out += sizeof(Node<Type>*);
			out += sizeof(alni);
			out += sizeof(Node<Type>) * length;
			return out;
		}

		alni sizeUsedMem() {
			return sizeAllocatedMem();
		}

		Iterator<Type> begin() const {
			return {top};
		}

		alni end() const {
			return NULL;
		}
	};

  template <typename Type, const halni tSize, typename Index = halni>
  class ConstSizeStack {

      Type mBuff[tSize];
      Index mLoadIdx = 0;

  public:

      ConstSizeStack() {}

      Index loadLen() const {
        return mLoadIdx;
      }

      void push(const Type& in) {
        assert(mLoadIdx >= 0 && mLoadIdx < tSize && "out of range");
        mBuff[mLoadIdx] = in;
        mLoadIdx++;
      }

      void pop() {
        mLoadIdx--;
      }

      Type& last() {
        assert(mLoadIdx >= 0 && mLoadIdx < tSize && "out of range");
        return mBuff[mLoadIdx - 1];
      }

      const Type& last() const {
        assert(mLoadIdx >= 0 && mLoadIdx < tSize && "out of range");
        return mBuff[mLoadIdx - 1];
      }

      Type* buff() {
        return mBuff;
      }

      Type& operator[] (Index idx) {
        assert(idx >= 0 && idx < tSize && "out of range");
        return mBuff[idx];
      }

      const Type* buff() const {
        return mBuff;
      }

      const Type& operator[] (Index idx) const {
        assert(idx >= 0 && idx < tSize && "out of range");
        return mBuff[idx];
      }

      constexpr Index len() {
        return tSize;
      }
  };
};