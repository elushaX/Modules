#include "Archiver.hpp"
#include "Map.hpp"
#include "Tests.hpp"

using namespace tp;

SUITE(HashTable) {
	TEST(SimpleReference) {
		tp::Map<tp::ualni, TestClass, TestAllocator> map;

		for (auto i : IterRange(1000, 100000)) {
			map.put(i, TestClass(i));
		}

		for (auto i : IterRange(1000, 100000)) {
			CHECK(map.presents(i));
			CHECK_EQUAL((tp::ualni) map.get(i).getVal(), (tp::ualni) i);
		}

		for (auto i : IterRange(1000, 100000)) {
			map.put(i, TestClass(i));
		}

		for (auto i : IterRange(1000, 2000)) {
			CHECK(map.presents(i));
			map.remove(i);
			CHECK(!map.presents(i));
		}

		for (auto i : IterRange(2000, 100000)) {
			CHECK(map.presents(i));
			CHECK_EQUAL((tp::ualni) map.get(i).getVal(), (tp::ualni) i);
		}

		for (auto i : map) {
			i->val.setVal(3);
		}

		map.removeAll();
	}

	TEST(SimplePointer) {
		tp::Map<tp::ualni, TestClass*, TestAllocator> map;

		for (auto i : IterRange(1000)) {
			map.put(i, new TestClass(i));
		}

		for (auto i : IterRange(1000)) {
			CHECK(map.presents(i));
			CHECK_EQUAL((tp::ualni) map.get(i)->getVal(), (tp::ualni) i);
		}

		for (auto i : IterRange(1000)) {
			auto del = map.get(i);
			map.put(i, new TestClass(i));
			delete del;
		}

		for (auto i : IterRange(900, 1000)) {
			CHECK(map.presents(i));
			delete map.get(i);
			map.remove(i);
			CHECK(!map.presents(i));
		}

		for (auto i : IterRange(900)) {
			CHECK(map.presents(i));
			CHECK_EQUAL((tp::ualni) map.get(i)->getVal(), (tp::ualni) i);
		}

		for (auto i : map) {
			i->val->setVal(3);
			delete i->val;
		}

		map.removeAll();
	}

	TEST(Copy) {
		tp::Map<tp::ualni, TestClass, TestAllocator> map;

		for (auto i : IterRange(10)) {
			map.put(i, TestClass(i));
		}

		tp::Map<tp::ualni, TestClass, TestAllocator> map2 = map;

		CHECK(map == map2);

		map.removeAll();
		map2.removeAll();
	}

	TEST(SaveLoad) {
		tp::Map<tp::ualni, TestClass, TestAllocator> map;

		for (auto i : IterRange(10)) {
			map.put(i, TestClass(i));
		}

		ArchiverExample<1024, false> write;
		ArchiverExample<1024, true> read;

		write % map;

		map.removeAll();

		CHECK(map.size() == 0);

		memCopy(read.mBuff, write.mBuff, sizeof(write.mBuff));

		read % map;

		CHECK(map.size() == 10);

		for (auto i : IterRange(10)) {
			CHECK(map.presents(i));
			CHECK_EQUAL(map.get(i).getVal(), i);
		}

		map.removeAll();
	}
}