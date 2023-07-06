
#include "containers.h"

using namespace tp;

#include <cstdio>

void list_test() {
	tp::List<int> q;

	q.pushBack(0);
	q.pushBack(1);
	q.pushBack(2);
	q.pushBack(3);
	q.pushBack(4);
	q.pushBack(5);

	for (auto node : q) {
		printf("%i ", node.data());
	}

	q.popBack();
	q.popFront();

	q.pushBack(4);
	q.pushFront(1);

	q.invert();

	for (auto node : q) {
		printf("%i ", node.data());
	}

	q.sort([](ListNode<int>* const& node1, ListNode<int>* const& node2) { return node1->data > node2->data; });

	for (auto node : q) {
		printf("%i ", node.data());
	}
}

// numeric node test
void avl_test() {
	AvlTree<NumericNode<float>, float> avl;

	for (auto i : Range<ualni>(100)) {
		//avl.insert(i, halnf(i));
		auto ret = avl.isInvalid(avl.head());
		DBG_BREAK(ret);
	}

	for (auto node : avl) {
		printf("%f ", node.node()->mVal);
	}

	auto p = avl.find(0);
	p = avl.find(50);
	//p = avl.find(avl_ts(100));

	auto t = avl.find_less_or_eq(50);
	t = avl.find_less_or_eq(50.5);
	t = avl.find_less_or_eq(100.5);
	t = avl.find_less_or_eq(0.5);

	auto end = avl.end(avl.find(99));
	for (auto iter = avl.begin(p); iter != end; ++iter) {
		printf("%f ", iter.node()->mVal);
	}
}

#include <typeinfo>

void map_test() {

	struct CheckData {
		ualni mDummy = 0;
		bool mAddedToMap = false;
	};

	HashMap<alni, alni> map;

	const uhalni test_scale = 1000;
	const uhalni test_iterations = 100000;

	Array<CheckData> checker(test_scale);

	for (auto test : Range<ualni>(test_iterations)) {
		auto idx = alni(tp::randf() * (test_scale - 1));

		auto map_idx = map.presents(idx);
		if (checker[idx].mAddedToMap != map_idx) {
			DBG_BREAK("Presentment mismatch");
		}

		if (map_idx) {
			if (idx != map.getSlotVal(map_idx)) {
				DBG_BREAK("Data mismatch");
			}
		}

		if (checker[idx].mAddedToMap) {
			map.remove(idx);
		}
		else {
			map.put(idx, idx);
		}

		checker[idx].mAddedToMap = !checker[idx].mAddedToMap;
	}

	for (auto iter : map) {
		iter->val = 0;
	}
}

void queue_test() {
	tp::Queue<int> q;

	q.push(0);
	q.push(1);
	q.push(2);
	q.push(3);
	q.push(4);
	q.push(5);

	for (auto node : q) {
		printf("%i ", node.mIter->data);
	}

	q.pop();
	q.pop();
	q.pop();
	q.pop();
	q.pop();
	q.pop();

	q.push(3);
	q.push(4);
	q.push(5);

	for (auto node : q) {
		printf("%i ", node.mIter->data);
	}
}

void timeline_test() {
	tp::DebugTimeline<tp::halni> tl(100);
	tl.add(1);
}