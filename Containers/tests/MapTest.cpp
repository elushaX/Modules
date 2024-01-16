#include "Archiver.hpp"
#include "Map.hpp"
#include "Testing.hpp"
#include "Tests.hpp"

using namespace tp;

TEST_DEF_STATIC(SimpleReference) {
	tp::Map<tp::ualni, TestClass, tp::HeapAlloc> map;

	for (auto i : Range(1000, 100000)) {
		map.put(i, TestClass(i));
	}

	for (auto i : Range(1000, 100000)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i).getVal(), i);
	}

	for (auto i : Range(1000, 100000)) {
		map.put(i, TestClass(i));
	}

	for (auto i : Range(1000, 2000)) {
		TEST(map.presents(i));
		map.remove(i);
		TEST(!map.presents(i));
	}

	for (auto i : Range(2000, 100000)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i).getVal(), i);
	}

	for (auto i : map) {
		i->val.setVal(3);
	}

	map.removeAll();
}

TEST_DEF_STATIC(SimplePointer) {
	tp::Map<tp::ualni, TestClass*, tp::HeapAlloc> map;

	for (auto i : Range(1000)) {
		map.put(i, new TestClass(i));
	}

	for (auto i : Range(1000)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i)->getVal(), i);
	}

	for (auto i : Range(1000)) {
		auto del = map.get(i);
		map.put(i, new TestClass(i));
		delete del;
	}

	for (auto i : Range(900, 1000)) {
		TEST(map.presents(i));
		delete map.get(i);
		map.remove(i);
		TEST(!map.presents(i));
	}

	for (auto i : Range(900)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i)->getVal(), i);
	}

	for (auto i : map) {
		i->val->setVal(3);
		delete i->val;
	}

	map.removeAll();
}

TEST_DEF_STATIC(Copy) {
	tp::Map<tp::ualni, TestClass, tp::HeapAlloc> map;

	for (auto i : Range(10)) {
		map.put(i, TestClass(i));
	}

	tp::Map<tp::ualni, TestClass, tp::HeapAlloc> map2 = map;

	TEST_EQUAL(map, map2);

	map.removeAll();
	map2.removeAll();
}

TEST_DEF_STATIC(SaveLoad) {
	tp::Map<tp::ualni, TestClass, tp::HeapAlloc> map;

	for (auto i : Range(10)) {
		map.put(i, TestClass(i));
	}

	ArchiverExample<1024, false> write;
	ArchiverExample<1024, true> read;

	write % map;

	map.removeAll();

	TEST(map.size() == 0);

	memCopy(read.mBuff, write.mBuff, sizeof(write.mBuff));

	read % map;

	TEST(map.size() == 10);

	for (auto i : Range(10)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i).getVal(), i);
	}

	map.removeAll();
}

TEST_DEF(Map) {
	testSimplePointer();
	testSimpleReference();
	testSaveLoad();
}
