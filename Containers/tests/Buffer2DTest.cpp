#include "NewPlacement.hpp"

#include "Tests.hpp"
#include "Testing.hpp"
#include "Buffer2D.hpp"

using namespace tp;

const ualni size = 1000;

TEST_DEF_STATIC(Simple1) {
	Buffer2D<int, HeapAlloc> buff;
	buff.reserve({ 4, 4 });
	buff.set( { 2, 2 }, 5);
	TEST(buff.get( { 2, 2 } ) == 5);
}

TEST_DEF_STATIC(Simple2) {
	// TEST(false);
}

TEST_DEF(Buffer2d) {
  testSimple1();
  testSimple2();
}