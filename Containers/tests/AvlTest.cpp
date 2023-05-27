
#include "Tests.hpp"

#include "AvlTree.hpp"

#include "Testing.hpp"

#include <iostream>

using namespace tp;

TEST_DEF_STATIC(Simple) {
  AvlTree<AvlNumericKey<alni>, TestClass, TestAllocator> tree;

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

TEST_DEF(Avl) {
  testSimple();
}