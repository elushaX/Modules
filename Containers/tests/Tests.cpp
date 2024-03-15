
#include "Tests.hpp"
#include "UnitTest++/UnitTest++.h"

void* TestAllocator::allocate(tp::ualni size) {
	count++;
	return malloc(size);
}

void TestAllocator::deallocate(void* p) {
	if (p) {
		free(p);
		count--;
	}
}


TestAllocator::~TestAllocator() { 
	ASSERT(!count);
}

int main() {
	return UnitTest::RunAllTests();
}
