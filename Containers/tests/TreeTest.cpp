
#include "Tests.hpp"
#include "Tree.hpp"
#include "Testing.hpp"

using namespace tp;

TEST_DEF_STATIC(Simple) {
	AvlTree<AvlNumericKey<alni>, TestClass, HeapAlloc> tree;

	TEST(tree.size() == 0);
	TEST(tree.head() == nullptr);

	tree.insert(6, TestClass(6));
	TEST(tree.isValid());
	TEST(tree.size() == 1);
	TEST(tree.head()->data == TestClass(6));

	tree.remove(6);
	TEST(tree.isValid());
	TEST(tree.size() == 0);
	TEST(tree.head() == nullptr);
}

TEST_DEF_STATIC(Persistance) {
	AvlTree<AvlNumericKey<alni>, TestClass, HeapAlloc> tree;

	const auto size = 1000;

	struct Item {
		Item() : data(0) {}
		bool presents = false;
		TestClass data;
	};

	Item buff[size];

	for (auto i : Range(size)) {
		buff[i].data.setVal(i);
	}

	// random load
	ualni loadSize = 0;
	while (loadSize < size / 2) {
		auto idx = ualni(randomFloat() * (size - 1));
		DEBUG_ASSERT(idx < size)
		if (!buff[idx].presents) {
			tree.insert((alni) buff[idx].data.getVal(), buff[idx].data);
			loadSize++;
			buff[idx].presents = true;

			TEST(tree.isValid());
			TEST(tree.size() == loadSize);
		}
	}

	for (auto& item : buff) {
		if (item.presents) continue;
		tree.insert((alni) item.data.getVal(), item.data);
		loadSize++;
		item.presents = true;

		TEST(tree.isValid());
		TEST(tree.size() == loadSize);
	}

	TEST(tree.size() == size);
	TEST(tree.maxNode(tree.head())->data.getVal() == size - 1);
	TEST(tree.minNode(tree.head())->data.getVal() == 0);

	// find
	for (auto item : buff) {
		auto node = tree.find((alni) item.data.getVal());
		TEST(node);
		if (!node) continue;
		TEST(node->data.getVal() == item.data.getVal());
	}

	TEST(!tree.find(size + 1));
	TEST(!tree.find(-1));

	// random unload
	ualni unloadSize = 0;
	while (unloadSize < size / 2) {
		auto idx = ualni(randomFloat() * (size - 1));
		if (buff[idx].presents) {

			tree.remove((alni) buff[idx].data.getVal());

			unloadSize++;
			buff[idx].presents = false;

			// find
			for (auto item : buff) {
				if (!item.presents) continue;
				auto node = tree.find((alni) item.data.getVal());
				TEST(node);
				if (!node) continue;
				TEST(node->data.getVal() == item.data.getVal());
			}

			TEST(tree.isValid());
			TEST(tree.size() == size - unloadSize);
		}
	}

	for (auto& item : buff) {
		if (item.presents) {
			tree.remove((alni) item.data.getVal());
			unloadSize++;
			item.presents = false;

			TEST(tree.isValid());
			TEST(tree.size() == size - unloadSize);
		}
	}

	TEST(tree.size() == 0);
	TEST(tree.head() == nullptr);
	TEST(tree.maxNode(tree.head()) == nullptr);
	TEST(tree.minNode(tree.head()) == nullptr);
}

TEST_DEF(Avl) {
	testSimple();
	testPersistance();
}