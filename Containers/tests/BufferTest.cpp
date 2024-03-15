#include "Buffer.hpp"
#include "Tests.hpp"

using namespace tp;

const ualni size = 1000;

SUITE(Buffer) {
	TEST(Simple1) {
		Buffer<TestClass, TestAllocator> buff;
		CHECK(buff.size() == 0);
		for (auto i : Range(size * 10)) {
			buff.append(TestClass(i));
		}
		CHECK(buff.size() == size * 10);
		while (buff.size())
			buff.pop();
		CHECK(buff.size() == 0);
	}

	TEST(Simple2) {
		Buffer<TestClass, TestAllocator> buff(size);
		CHECK(buff.size() == size);
		for (auto i : Range(size * 10))
			buff.append(TestClass(i));
		CHECK(buff.size() == size + size * 10);
		while (buff.size())
			buff.pop();
		CHECK(buff.size() == 0);
	}

	TEST(NO_TEST) { CHECK(false); }
}