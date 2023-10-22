#include "NewPlacement.hpp"

#include "Strings.hpp"
#include "Tests.hpp"

using namespace tp;

struct TestStruct {
	String str = "test data";
};

TEST_DEF_STATIC(Addition) {
	String str1 = "abc";
	String str2 = "def";
	{ str1 + str2; }
	{
		TEST(str1 + str2 == "abcdef");
		TEST(str1 + "aaaccc" == "abcaaaccc");
	}
}

TEST_DEF_STATIC(Simple) {
	String str;
	TEST(str.getIsConstFlag());
	TEST(str.getReferenceCount() == 2);

	String str2;
	TEST(str.getIsConstFlag());
	TEST(str.getReferenceCount() == 3);

	auto tmp = new TestStruct();
	TEST(tmp->str.getIsConstFlag());
	TEST(tmp->str.getReferenceCount() == 1);

	str = tmp->str;

	TEST(str.getIsConstFlag());
	TEST(str.getReferenceCount() == 2);

	delete tmp;

	TEST(str.getIsConstFlag());
	TEST(str.getReferenceCount() == 1);
}

TEST_DEF(Strings) {
	testSimple();
	testAddition();
}