#include "Archiver.hpp"
#include "Map.hpp"
#include "Tests.hpp"

#include <chrono>

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

	TEST(RemovingKeysTakesLinearTime) {
		tp::Map<tp::ualni, tp::ualni> map;
		const tp::ualni count = 200000;
		for (tp::ualni key = 0; key < count; key++)
			map.put(key, key);
		const auto start = std::chrono::steady_clock::now();
		for (tp::ualni key = 0; key < count / 2; key++)
			map.remove(key);
		const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		CHECK(ms < 200.0);
		CHECK_EQUAL(count / 2, map.size());
		for (tp::ualni key = count / 2; key < count; key += 997)
			CHECK_EQUAL(key, map.get(key));
	}

	TEST(MostlyEmptyMapShrinks) {
		tp::Map<tp::ualni, tp::ualni> map;
		for (tp::ualni key = 0; key < 4096; key++)
			map.put(key, key);
		const tp::ualni slots = map.end();
		for (tp::ualni key = 0; key < 4000; key++)
			map.remove(key);
		CHECK(map.end() < slots);
		for (tp::ualni key = 4000; key < 4096; key++)
			CHECK_EQUAL(key, map.get(key));
	}

	TEST(LookupsTerminateAfterMassRemoval) {
		tp::Map<tp::ualni, bool> map;
		for (tp::ualni key = 0; key < 1000; key++)
			map.put(key, true);
		for (tp::ualni key = 0; key < 990; key++)
			map.remove(key);
		for (tp::ualni key = 990; key < 1000; key++)
			map.remove(key);
		for (tp::ualni key = 0; key < 100; key++)
			CHECK(!map.presents(key + 5000).isValid());
		CHECK_EQUAL(0u, map.size());
	}
}
