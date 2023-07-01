
#include "Tests.hpp"

#include "Buffer.hpp"

#include "Testing.hpp"

#include <iostream>

using namespace tp;

const ualni size = 1000;

TEST_DEF_STATIC(Simple1) {
  Buffer<TestClass> buff;
  TEST(buff.size() == 0);
  for (auto i : Range(size * 10)) {
    buff.append(TestClass(i));
  }
  TEST(buff.size() == size * 10);
  while (buff.size()) buff.pop();
  TEST(buff.size() == 0);
}

TEST_DEF_STATIC(Simple2) {
  Buffer<TestClass> buff(size);
  TEST(buff.size() == 0);
  for (auto i : Range(size * 10)) buff.append(TestClass(i));
  TEST(buff.size() == size * 10);
  while (buff.size()) buff.pop();
  TEST(buff.size() == 0);
}

TEST_DEF(Buffer) {
  testSimple1();
  testSimple2();
}