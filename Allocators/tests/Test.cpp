
#include "UnitTest++/UnitTest++.h"

#include "Allocators.hpp"
#include "Utils.hpp"

#include <cmath>

using namespace tp;

struct TestStruct {
	alni val = 0;

	TestStruct() :
		val(0) {}
	explicit TestStruct(alni val) :
		val(val) {}
	TestStruct(const TestStruct& in) :
		val(in.val) {}

	~TestStruct() { val = -1; }

	bool operator==(const TestStruct& in) const { return in.val == val; }
};

template <alni tSize, class tAllocator>
class TestBenches {

	tAllocator mAlloc{};
	TestStruct mData[tSize]{};
	TestStruct* mLoaded[tSize]{};
	bool mIsLoaded[tSize]{};
	alni mLoadedNum = 0;

public:
	TestBenches() {
		for (alni i = 0; i < tSize; i++) {
			mData[i].val = i;
			mIsLoaded[i] = false;
			mLoaded[i] = nullptr;
		}
	}

	void runTests() {
		try {
			test1();
			test2();
			test3();
			test4();
			test5();
			test6();
		} catch (...) {
			ASSERT(false)
		}
	}

private:
	alni randomIdx(bool state, Range<alni> range = { 0, tSize }) {
	RAND:
		auto idx = alni(alnf(range.idxBegin()) + randomFloat() * alnf(range.idxDiff() + 1));
		idx = clamp(idx, alni(0), tSize - 1);
		if (state == mIsLoaded[idx]) goto RAND;
		return idx;
	}

	void verifyIntegrity() {
		mAlloc.checkValid();
		ASSERT(!mAlloc.checkWrap())
		for (alni i = 0; i < tSize; i++) {
			if (mIsLoaded[i]) {
				ASSERT(*mLoaded[i] == mData[i])
			}
		}
	}

	void loadItem(alni idx) {
		if (mIsLoaded[idx]) return;
		verifyIntegrity();
		mLoaded[idx] = new (mAlloc.allocate(sizeof(TestStruct))) TestStruct(mData[idx]);
		ASSERT(mLoaded[idx]);
		mIsLoaded[idx] = true;
		mLoadedNum++;
		verifyIntegrity();
	}

	void unloadItem(alni idx) {
		if (!mIsLoaded[idx]) return;
		verifyIntegrity();
		mLoaded[idx]->~TestStruct();
		mAlloc.deallocate(mLoaded[idx]);
		mIsLoaded[idx] = false;
		mLoadedNum--;
		verifyIntegrity();
	}

	void changeStates(Range<alni> rg, bool load, bool reversed = false, bool random = false) {
		for (auto i : rg) {
			alni idx = i;
			if (random) {
				idx = randomIdx(load, rg);
			} else if (reversed) {
				idx = rg.idxEnd() - i - 1;
			}
			(load) ? loadItem(idx) : unloadItem(idx);
		}
	}

	// full down-up load then up-down unload
	void test1() {
		changeStates({ 0, tSize }, true);
		changeStates({ 0, tSize }, false, true);
	}

	// full down-up load then down-up unload
	void test2() {
		changeStates({ 0, tSize }, true);
		changeStates({ 0, tSize }, false);
	}

	// full random load then random unload
	void test3() {
		changeStates({ 0, tSize }, true, false, true);
		changeStates({ 0, tSize }, false, false, true);
	}

	// combo tests 1-3
	void test4() {
		test1();
		test1();

		test2();
		test2();

		test3();
		test3();
	}

	static alnf sineUpFunction(alnf aSize, alnf aX, bool aReverse) {
		alnf end = 4 * 3.14159;
		alnf a = (2 / 7.f) * aSize;
		alnf b = end / aSize;

		alni c = ((-1 * aReverse) + (1 * !aReverse));
		alnf c1 = (aX - (end * aReverse)) / b;
		alnf c2 = (a * sin(aX - (end * aReverse)));
		alnf out = c1 + c2;
		return (alnf) c * out;
	}

	// sin load & sin unload with ~1/2 drop factor
	void test5() {
		alnf end = 4 * 3.14159;
		alnf step = end / 4.f;

		for (char i = 0; i < 2; i++) {
			for (alnf x = 0; x <= end; x += step) {

				alni target_alloc_count = (alni) ceil(sineUpFunction(tSize, x, i));
				target_alloc_count = clamp(target_alloc_count, alni(0), tSize);

				while (mLoadedNum > target_alloc_count) {
					unloadItem(randomIdx(0));
				}
				while (mLoadedNum < target_alloc_count) {
					loadItem(randomIdx(1));
				}
			}
		}
	}

	void checkWrap(ualni offset, bool after) {
		offset = clamp(offset, (ualni) 1, (ualni) MEM_WRAP_SIZE);

		TestStruct* ts = mLoaded[randomIdx(0)];
		ualni shift = (sizeof(TestStruct) * after) + (offset - 1) * after - offset * (!after);
		uint1* address = (((uint1*) ts) + shift);

		uint1 val = *address;
		*address = 5;

		ASSERT(!mAlloc.checkWrap());

		*address = val;
	}

	// mem guards test
	void test6() {
		changeStates({ 0, tSize }, 1);
#ifdef MEM_DEBUG
		for (alni after = 0; after < 2; after++) {
			for (alni offset = 1; offset <= MEM_WRAP_SIZE; offset++) {
				checkWrap(offset, after);
			}
		}
#endif
		changeStates({ 0, tSize }, 0);
	}
};

const ualni size = 500;

template <typename Alloc>
void testAlloc() {
	try {
		TestBenches<size, Alloc> heapTests{};
		heapTests.runTests();
	} catch (...) {
		ASSERT(false);
	}
}

SUITE(Allocators) {
	TEST(GlobalHeap) { testAlloc<HeapAllocGlobal>(); }

	TEST(Heap) { testAlloc<tp::HeapAlloc>(); }

	TEST(Chunk) {
		testAlloc<ChunkAlloc<TestStruct, size>>();
		testAlloc<ChunkAlloc<TestStruct, size * 2>>();
	}

	TEST(Pool) {
		testAlloc<PoolAlloc<TestStruct, 1>>();
		testAlloc<PoolAlloc<TestStruct, size / 100>>();
		testAlloc<PoolAlloc<TestStruct, size>>();
	}

	TEST(Simple) {
		auto a = new TestStruct(-1);
		delete a;
	}
}


int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleAllocators, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("AllocatorsTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	bool res = UnitTest::RunAllTests();

	testModule.deinitialize();

	return res;
}