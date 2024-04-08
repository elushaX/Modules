#pragma once

#include "Tree.hpp"

namespace tp {

	template <typename tType>
	class IntervalKey {
	public:
		IntervalKey() = default;

		IntervalKey(tType start, tType end) {
			mStart = start;
			mEnd = end;
		}

		inline bool descentRight(const IntervalKey& in) const {
			if (in.mStart != mStart) return in.mStart > mStart;
			return in.mEnd > mEnd;
		}

		inline bool exactNode(const IntervalKey& in) const { return in.mStart == mStart && in.mEnd == mEnd; }

		template <typename tTreeNodeType>
		inline const IntervalKey& getFindKey(const tTreeNodeType* node) const {
			return *this;
		}

		inline const IntervalKey& keyInRightSubtree(const IntervalKey& in) const { return in; }
		inline const IntervalKey& keyInLeftSubtree(const IntervalKey& in) const { return in; }

		template <typename tTreeNodeType>
		inline void updateNodeCache(const tTreeNodeType* node) {
			mMax = 0;
			if (node->mRight && node->mRight->key.mMax > mMax) mMax = node->mRight->key.mMax;
			if (node->mLeft && node->mLeft->key.mMax > mMax) mMax = node->mLeft->key.mMax;
			if (mMax < mEnd) mMax = mEnd;
		}

	public:
		tType mStart = tType();
		tType mEnd = tType();
		tType mMax = tType();
	};

	template <typename tType, typename tData>
	class IntervalTree : public AvlTree<IntervalKey<tType>, tData> {
		typedef typename AvlTree<IntervalKey<tType>, tData>::Node Node;

	public:
		IntervalTree() = default;

		template <typename tFunctor>
		ualni forEachIntersection(tType start, tType end, tFunctor functor) const {
			ualni debug = 0;
			forEachIntersectionUtil(this->head(), start, end, functor, debug);
			return debug;
		}

	private:
		template <typename tFunctor>
		void forEachIntersectionUtil(const Node* node, tType start, tType end, tFunctor functor, ualni& debug) const {
			if (node->mHeight == -1) return; // sentinel node (null node)

			debug++;

			// If 'start' is to the right of the rightmost point of any interval
			// in this node and all children, there won't be any matches.
			if (start > node->key.mMax) return;

			// Search left children
			forEachIntersectionUtil(node->mLeft, start, end, functor, debug);

			// Check this node
			if (start <= node->key.mEnd && end >= node->key.mStart) {
				functor(node->key.mStart, node->key.mEnd, node->data);
			}

			// If end is to the left of the start of this interval,
			// then it can't be in any child to the right.
			if (end < node->key.mStart) return;

			// Otherwise, search right children
			forEachIntersectionUtil(node->mRight, start, end, functor, debug);
		}
	};
}