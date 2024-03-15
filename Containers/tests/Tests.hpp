#pragma once

#include "UnitTest++/UnitTest++.h"

#include "Utils.hpp"

// counting number of allocations and deallocations
struct TestAllocator {
	TestAllocator() { count = 0; };

	tp::ualni count = 0;

	void* allocate(tp::ualni size);
	void deallocate(void*);

	~TestAllocator();
};

class TestClass {
	tp::ualni val2 = 0;
	tp::ualni val1;

public:
	TestClass() { val1 = 0; }

	explicit TestClass(tp::ualni val) :
		val1(val) {}

	template <class Saver>
	void write(Saver& file) const {
		file.write(val1);
	}

	template <class Loader>
	void read(Loader& file) {
		file.read(val1);
	}

	[[nodiscard]] bool operator==(const TestClass& in) const { return in.val1 == val1; }

	[[nodiscard]] tp::ualni getVal() const { return val1; }
	void setVal(tp::ualni val) { val1 = val; }
};