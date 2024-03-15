#include "Archiver.hpp"
#include "Tests.hpp"
#include "List.hpp"

using namespace tp;

SUITE(DoubleLinkedList) {

	TEST(SimpleReference) {
		tp::List<TestClass, TestAllocator> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };

		list.pushBack(TestClass(5));
		list.pushFront(TestClass(0));

		ualni i = -1;
		for (auto iter : list) {
			i++;
			CHECK_EQUAL(iter->getVal(), i);
		}

		CHECK(i == 5);

		list.removeAll();
	}

	TEST(SimplePointer) {
		tp::List<TestClass*, TestAllocator> list = {
			new TestClass(1), new TestClass(2), new TestClass(3), new TestClass(4)
		};

		list.pushBack(new TestClass(5));
		list.pushFront(new TestClass(0));

		ualni i = -1;
		for (auto iter : list) {
			i++;
			CHECK_EQUAL(iter->getVal(), i);
		}

		CHECK(i == 5);

		for (auto iter : list) {
			delete iter.data();
		}

		list.removeAll();
	}

	TEST(Copy) {
		tp::List<TestClass, TestAllocator> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };
		tp::List<TestClass, TestAllocator> list2 = list;

		CHECK(list == list2);

		list.removeAll();
		list2.removeAll();
	}

	TEST(Serialization) {
		tp::List<TestClass, TestAllocator> list = { TestClass(1), TestClass(2), TestClass(3), TestClass(4) };

		ArchiverExample<1024, false> write;
		ArchiverExample<1024, true> read;

		write % list;

		list.removeAll();

		memCopy(read.mBuff, write.mBuff, sizeof(write.mBuff));

		read % list;

		ualni i = 0;
		for (auto iter : list) {
			i++;
			CHECK(iter->getVal() == i);
		}
		CHECK(i == 4);

		list.removeAll();
	}
}