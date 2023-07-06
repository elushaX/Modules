#pragma once

#include "allocators.h"

namespace tp {
	extern ModuleManifest gModuleContainer;

	template <typename Type>
	class Queue {
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
			IterType& data() { return mIter->data; }
			const Iterator& operator*() { return *this; }
			inline void operator++() { mIter = mIter->below; }
			bool operator==(const Iterator& end) { return mIter == NULL; }
			bool operator!=(const Iterator& end) { return mIter != NULL; }
		};

		alni length;
		Node<Type>* top;
		Node<Type>* bottom;

		Queue() {
			MODULE_SANITY_CHECK(gModuleContainer);
			length = 0;
			top = nullptr;
			bottom = nullptr;
		}

		~Queue() {
			free();
		}

		// pushes the node on the bottom of the stack
		void push(Type data) {
			Node<Type>* new_node = new Node<Type>(data, NULL);

			if (bottom) {
				bottom->below = new_node;
			} else {
				top = new_node;
			}

			bottom = new_node;

			length++;
		}

		// pops the top node of the stack
		void pop() {
			assert(top);
			Node<Type>* del = top;
			top = top->below;
			delete del;
			length--;
			
			if (!top) {
				bottom = NULL;
			}
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
			bottom = NULL;
		}

		alni sizeAllocatedMem() {
			alni out = 0;
			out += sizeof(Node<Type>*) * 2;
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

		Iterator<Type> end() const {
			return {};
		}
	};
};