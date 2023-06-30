
#include "Tests.hpp"
#include "Testing.hpp"

#include "Map.hpp"

#include <iostream>

using namespace tp;

TEST_DEF_STATIC(SimpleReference) {
	tp::Map<tp::ualni, TestClass, TestAllocator> map;

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

	TEST(map.getAllocator().getAllocationsCount()  == 1);
}

TEST_DEF_STATIC(SimplePointer) {
	tp::Map<tp::ualni, TestClass*, TestAllocator> map;

	for (auto i : Range(1000)) {
		map.put(i, new TestClass(i));
	}

	for (auto i : Range(1000)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i)->getVal(), i);
	}

	for (auto i : Range(1000)) {
		map.put(i, new TestClass(i));
	}

	for (auto i : Range(900, 1000)) {
		TEST(map.presents(i));
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

	TEST(map.getAllocator().getAllocationsCount()  == 1);
}

TEST_DEF_STATIC(Copy) {
	tp::Map<tp::ualni, TestClass, TestAllocator> map;

	for (auto i : Range(10)) {
		map.put(i, TestClass(i));
	}

	tp::Map<tp::ualni, TestClass, TestAllocator> map2 = map;

	TEST_EQUAL(map, map2);

	map.removeAll();
	map2.removeAll();

	TEST(map.getAllocator().getAllocationsCount()  == 1);
	TEST(map2.getAllocator().getAllocationsCount()  == 1);
}

TEST_DEF_STATIC(SaveLoad) {
	tp::Map<tp::ualni, TestClass, TestAllocator> map;

	for (auto i : Range(10)) {
		map.put(i, TestClass(i));
	}

	TestFile file;

	map.write(file);

	map.removeAll();

	TEST(map.getAllocator().getAllocationsCount()  == 1);

	file.setAddress(0);

	map.read(file);

	TEST(map.getAllocator().getAllocationsCount()  == 11);

	for (auto i : Range(10)) {
		TEST(map.presents(i));
		TEST_EQUAL(map.get(i).getVal(), i);
	}

	map.removeAll();

	TEST(map.getAllocator().getAllocationsCount()  == 1);
}

TEST_DEF(Map) {
	testSimplePointer();
	testSimpleReference();
	testSaveLoad();
}