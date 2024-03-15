
#include "UnitTest++/UnitTest++.h"

#include "Strings.hpp"
#include "StringLogic.hpp"

SUITE(Strings) {
	using namespace tp;

	typedef StringLogic<char> Logic;

	struct TestStruct {
		String str = "test data";
	};

	TEST(CoreLogic) {
		String str;
		CHECK(str.getIsConstFlag());
		CHECK(str.getReferenceCount() == 2);

		String str2;
		CHECK(str.getIsConstFlag());
		CHECK(str.getReferenceCount() == 3);

		auto tmp = new TestStruct();
		CHECK(tmp->str.getIsConstFlag());
		CHECK(tmp->str.getReferenceCount() == 1);

		str = tmp->str;

		CHECK(str.getIsConstFlag());
		CHECK(str.getReferenceCount() == 2);

		delete tmp;

		CHECK(str.getIsConstFlag());
		CHECK(str.getReferenceCount() == 1);
	}

	TEST(Addition) {
		String str1 = "abc";
		String str2 = "def";
		{ str1 + str2; }
		{
			CHECK(str1 + str2 == "abcdef");
			CHECK(str1 + "aaaccc" == "abcaaaccc");
		}
	}

	TEST(isNewLineChar) {
		CHECK(Logic::isNewLineChar('\n'));
		CHECK(Logic::isNewLineChar('\r'));
		CHECK(!Logic::isNewLineChar('a'));
	}

	TEST(isEndChar) {
		CHECK(Logic::isEndChar('\0'));
		CHECK(!Logic::isEndChar('a'));
	}

	TEST(calcLength) {
		const char* str1 = "Hello";
		const char* str2 = "Wor\nld2";
		CHECK(Logic::calcLength(str1) == 5);
		CHECK(Logic::calcLength(str2) == 7);
	}

	TEST(insertLogic_emptyTarget) {
		char inserted[40] = { "" };
		const char* cur = "Hello";
		const char* tar = "";
		const char* result = "Hello";
		Logic::insertLogic(inserted, 40, cur, tar, 5, 0);
		CHECK(Logic::isEqualLogic(inserted, result));
	}

	TEST(insertLogic_emptyCurrent) {
		char inserted[40] = { "" };
		const char* cur = "";
		const char* tar = "World";
		const char* result = "World";
		Logic::insertLogic(inserted, 40, cur, tar, 0, 5);
		CHECK(Logic::isEqualLogic(inserted, result));
	}

	TEST(insertLogic_curLengthLessThanPos) {
		char inserted[40] = { "" };
		const char* cur = "Hello";
		const char* tar = "World";
		const char* result = "HelloWorld";
		Logic::insertLogic(inserted, 40, cur, tar, 5, 5);
		CHECK(Logic::isEqualLogic(inserted, result));
	}

	TEST(removeLogic_emptyCurrent) {
		char removed[40] = { "" };
		const char* cur = "";
		const char* result = "";
		Logic::removeLogic(removed, 40, cur, 0, 0);
		CHECK(Logic::isEqualLogic(removed, result));
	}

	TEST(removeLogic_removeUntilEnd) {
		char removed[40] = { "" };
		const char* cur = "HelloWorld";
		const char* result = "World";
		Logic::removeLogic(removed, 40, cur, 0, 5);
		CHECK(Logic::isEqualLogic(removed, result));
	}

	TEST(removeLogic_removeMiddleCharacters) {
		char removed[40] = { "" };
		const char* cur = "HelloWorld";
		const char* result = "Helrld";
		Logic::removeLogic(removed, 40, cur, 3, 7);
		CHECK(Logic::isEqualLogic(removed, result));
	}

	TEST(insertLogicGood) {
		char inserted[40] = { "" };
		const char* cur = "Hello";
		const char* tar = "World";
		const char* result = "HelloWorld";
		Logic::insertLogic(inserted, 40, cur, tar, 5, 5);
		CHECK(Logic::isEqualLogic(inserted, result));
	}

	TEST(removeLogicGood) {
		char removed[40] = { "" };
		const char* cur = "HelloWorld";
		const char* result = "Helld";
		Logic::removeLogic(removed, 40, cur, 3, 8);
		CHECK(Logic::isEqualLogic(removed, result));
	}

	TEST(convertStringToValue_alni) {
		const char* str1 = "123";
		const char* str2 = "456xyz";
		const char* str3 = "789";
		alni output;
		CHECK(Logic::convertStringToValue(str1, output) && output == 123);
		CHECK(!Logic::convertStringToValue(str2, output));
		CHECK(Logic::convertStringToValue(str3, output) && output == 789);
	}

	TEST(convertStringToValue_alnf) {
		const char* str1 = "12.34";
		const char* str2 = "56.78xyz";
		const char* str3 = "90.12";
		alnf output;
		CHECK(Logic::convertStringToValue(str1, output) && output == 12.34);
		CHECK(!Logic::convertStringToValue(str2, output));
		CHECK(Logic::convertStringToValue(str3, output) && output == 90.12);
	}

	TEST(convertStringToValue_bool) {
		const char* str1 = "true";
		const char* str2 = "false";
		const char* str3 = "1";
		const char* str4 = "0";
		bool output;
		CHECK(Logic::convertStringToValue(str1, output) && output == true);
		CHECK(Logic::convertStringToValue(str2, output) && output == false);
		CHECK(Logic::convertStringToValue(str3, output) && output == true);
		CHECK(Logic::convertStringToValue(str4, output) && output == false);
	}

	TEST(convertValueToString_alni) {
		alni input1 = 123;
		alni input2 = -456;
		char output[10];
		CHECK(Logic::convertValueToString(input1, output, 10));
		CHECK(Logic::isEqualLogic(output, "123"));

		CHECK(Logic::convertValueToString(input2, output, 10));
		CHECK(Logic::isEqualLogic(output, "-456"));
	}

	TEST(convertValueToString_alnf) {
		alnf input1 = 12.34;
		alnf input2 = -56.78;
		char output[10];

		CHECK(Logic::convertValueToString(input1, output, 10));
		CHECK(Logic::isEqualLogic(output, "12.340000"));

		CHECK(Logic::convertValueToString(input2, output, 10));
		CHECK(Logic::isEqualLogic(output, "-56.78000"));
	}

	TEST(convertValueToString_bool) {
		bool input1 = true;
		bool input2 = false;
		char output[10];

		CHECK(Logic::convertValueToString(input1, output, 10));
		CHECK(Logic::isEqualLogic(output, "true"));

		CHECK(Logic::convertValueToString(input2, output, 10));
		CHECK(Logic::isEqualLogic(output, "false"));
	}

	TEST(isEqualLogic) {
		const char* str1 = "Hello";
		const char* str2 = "Hello";
		const char* str3 = "World";
		const char* str4 = "Hello2";
		CHECK(Logic::isEqualLogic(str1, str2));
		CHECK(!Logic::isEqualLogic(str1, str4));
		CHECK(!Logic::isEqualLogic(str1, str3));
	}

	TEST(calcLineOffsets) {
		const char* str = "\n\nHello\nWorld\n\n ";
		Buffer<Logic::Index> offsets;
		Logic::calcLineOffsets(str, Logic::calcLength(str), offsets);

		CHECK(offsets.size() == 7);

		CHECK(offsets[0] == 0);
		CHECK(offsets[1] == 1);
		CHECK(offsets[2] == 2);
		CHECK(offsets[3] == 8);
		CHECK(offsets[4] == 14);
		CHECK(offsets[5] == 15);
		CHECK(offsets[6] == 16);
	}
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleStrings, &tp::gModuleUtils, nullptr };
	tp::ModuleManifest testModule("StringsTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	auto res = UnitTest::RunAllTests();

	testModule.deinitialize();

	return res;
}