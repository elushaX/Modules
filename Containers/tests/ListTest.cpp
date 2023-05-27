
#include "Tests.hpp"
#include "Testing.hpp"

#include <iostream>

using namespace tp;

TEST_DEF_STATIC(SimpleReference) {
  tp::List<TestClass, TestAllocator> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };

  list.pushBack(TestClass(5));
  list.pushFront(TestClass(0));

  ualni i = -1;
  for (auto iter : list) {
    i++;
    TEST_EQUAL(iter->getVal(), i);
  }

  TEST(i == 5);

  list.removeAll();
  TEST(list.getAllocator().getAllocationsCount() == 0);
}

TEST_DEF_STATIC(SimplePointer) {
  tp::List<TestClass*, TestAllocator> list = { new TestClass(1), new TestClass(2), new TestClass(3), new TestClass(4) };

  list.pushBack(new TestClass(5));
  list.pushFront(new TestClass(0));

  ualni i = -1;
  for (auto iter : list) {
    i++;
    TEST_EQUAL(iter->getVal(), i);
  }

  TEST(i == 5);

  list.removeAll();

  TEST(list.getAllocator().getAllocationsCount()  == 0);
}

TEST_DEF_STATIC(Copy) {
  tp::List<TestClass, TestAllocator> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };
  tp::List<TestClass, TestAllocator> list2 = list;

  TEST_EQUAL(list, list2);

  list.removeAll();
  list2.removeAll();

  TEST(list.getAllocator().getAllocationsCount()  == 0);
  TEST(list2.getAllocator().getAllocationsCount()  == 0);
}

TEST_DEF_STATIC(SaveLoad) {
  tp::List<TestClass, TestAllocator> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };

  TestFile file;

  list.write(file);

  list.removeAll();

  file.setAddress(0);

  list.read(file);

  ualni i = 0;
  for (auto iter : list) {
    i++;
    TEST_EQUAL(iter->getVal(), i);
  }
  TEST(i == 4);

  list.removeAll();

  TEST(list.getAllocator().getAllocationsCount()  == 0);
}

TEST_DEF(List) {
  testSimplePointer();
  testSimpleReference();
  testSaveLoad();
}