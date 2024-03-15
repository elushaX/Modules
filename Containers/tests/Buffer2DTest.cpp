#include "Buffer2D.hpp"
#include "Tests.hpp"

using namespace tp;

const ualni size = 1000;

SUITE(Buffer2D) {
	TEST(Simple) {
		Buffer2D<int, TestAllocator> buff;
		buff.reserve({ 4, 4 });
		buff.set({ 2, 2 }, 5);
		CHECK(buff.get({ 2, 2 }) == 5);
	}

	TEST(NO_TESTS) {
		CHECK(false);
	}
}
