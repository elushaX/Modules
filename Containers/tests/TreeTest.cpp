#include "Tests.hpp"
#include "Tree.hpp"

using namespace tp;

SUITE(AvlTree) {
	TEST(Simple) {
		AvlTree<AvlNumericKey<alni>, TestClass, TestAllocator> tree;

		CHECK(tree.size() == 0);
		CHECK(tree.head() == nullptr);

		tree.insert(6, TestClass(6));
		CHECK(tree.isValid());
		CHECK(tree.size() == 1);
		CHECK(tree.head()->data == TestClass(6));

		tree.remove(6);
		CHECK(tree.isValid());
		CHECK(tree.size() == 0);
		CHECK(tree.head() == nullptr);
	}

	TEST(Persistance) {
		AvlTree<AvlNumericKey<alni>, TestClass, TestAllocator> tree;

		const auto size = 10;

		struct Item {
			Item() :
				data(0) {}
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

				CHECK(tree.isValid());
				CHECK(tree.size() == loadSize);
			}
		}

		for (auto& item : buff) {
			if (item.presents) continue;
			tree.insert((alni) item.data.getVal(), item.data);
			loadSize++;
			item.presents = true;

			CHECK(tree.isValid());
			CHECK(tree.size() == loadSize);
		}

		CHECK(tree.size() == size);
		CHECK(tree.maxNode(tree.head())->data.getVal() == size - 1);
		CHECK(tree.minNode(tree.head())->data.getVal() == 0);

		// find
		for (auto item : buff) {
			auto node = tree.find((alni) item.data.getVal());
			CHECK(node);
			if (!node) continue;
			CHECK(node->data.getVal() == item.data.getVal());
		}

		CHECK(!tree.find(size + 1));
		CHECK(!tree.find(-1));

		// random unload
		ualni unloadSize = 0;
		while (unloadSize < size / 2) {
			auto idx = ualni(randomFloat() * (size - 1));
			if (buff[idx].presents) {

				tree.remove((alni) buff[idx].data.getVal());

				CHECK(tree.isValid());

				unloadSize++;
				buff[idx].presents = false;

				// find
				for (auto item : buff) {
					if (!item.presents) continue;
					auto node = tree.find((alni) item.data.getVal());
					CHECK(node);
					if (!node) continue;
					CHECK(node->data.getVal() == item.data.getVal());
				}

				CHECK(tree.isValid());
				CHECK(tree.size() == size - unloadSize);
			}
		}

		for (auto& item : buff) {
			if (item.presents) {
				tree.remove((alni) item.data.getVal());
				unloadSize++;
				item.presents = false;

				CHECK(tree.isValid());
				CHECK(tree.size() == size - unloadSize);
			}
		}

		CHECK(tree.size() == 0);
		CHECK(tree.head() == nullptr);
		CHECK(tree.maxNode(tree.head()) == nullptr);
		CHECK(tree.minNode(tree.head()) == nullptr);
	}
}