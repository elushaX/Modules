#pragma once

#include "Utils.hpp"

class TestClass {
  tp::ualni val2 = 0;
  tp::ualni val1;
public:

  explicit TestClass(tp::ualni val) : val1(val) {}

  template<class Saver>
  void write(Saver& file) const {
    file.write(val1);
  }

  template<class Loader>
  void read(Loader& file) {
    file.read(val1);
  }

  [[nodiscard]] bool operator==(const TestClass& in) const {
    return in.val1 == val1;
  }

  [[nodiscard]] tp::ualni getVal() const { return val1; }
  void setVal(tp::ualni val) { val1 = val; }
};

class TestAllocator {
  tp::ualni nAllocations = 0;
public:
  TestAllocator() = default;
  void* allocate(tp::ualni size);
  void deallocate(void* p);
  [[nodiscard]] tp::ualni getAllocationsCount() const;
};

class TestFile {
  tp::ualni mem[1024] = { 0 };
  tp::ualni address = 0;

public:
  TestFile() = default;

  template<typename Type>
  void write(const Type& val) {
    val.write(*this);
  }

  template<>
  void write<tp::ualni>(const tp::ualni& val) {
    mem[address] = val;
    address++;
  }

  void setAddress(tp::ualni addr) { address = addr; }

  template<typename Type>
  void read(Type& val) {
    val.read(*this);
  }

  template<>
  void read<tp::ualni>(tp::ualni& val) {
    val = mem[address];
    address++;
  }
};

void testList();
void testMap();
void testAvl();