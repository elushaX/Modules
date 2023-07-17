
#include "Tests.hpp"
#include "StringLogic.hpp"

using namespace tp;

typedef StringLogic<char> Logic;


TEST_DEF(isNewLineChar) {
	// Test isNewLineChar function
	TEST(Logic::isNewLineChar('\n'));
	TEST(Logic::isNewLineChar('\r'));
	TEST(!Logic::isNewLineChar('a'));
}

TEST_DEF(isEndChar) {
	// Test isEndChar function
	TEST(Logic::isEndChar('\0'));
	TEST(!Logic::isEndChar('a'));
}

TEST_DEF(calcLength) {
	// Test calcLength function
	const char* str1 = "Hello";
	const char* str2 = "Wor\nld2";
	TEST(Logic::calcLength(str1) == 5);
	TEST(Logic::calcLength(str2) == 7);
}

// ------------------------------- Inserting / Removing ----------------------------------------- //

TEST_DEF(insertLogic_emptyTarget) {
	// Test insertLogic function with an empty target string
	const char* cur = "Hello";
	const char* tar = "";
	const char* result = "Hello";
	char* inserted = Logic::insertLogic(cur, tar, 5, 0);
	TEST(Logic::isEqualLogic(inserted, result));
	delete[] inserted;
}

TEST_DEF(insertLogic_emptyCurrent) {
	// Test insertLogic function with an empty current string
	const char* cur = "";
	const char* tar = "World";
	const char* result = "World";
	char* inserted = Logic::insertLogic(cur, tar, 0, 5);
	TEST(Logic::isEqualLogic(inserted, result));
	delete[] inserted;
}

TEST_DEF(insertLogic_curLengthLessThanPos) {
	// Test insertLogic function with current length less than the insert position
	const char* cur = "Hello";
	const char* tar = "World";
	const char* result = "HelloWorld";
	char* inserted = Logic::insertLogic(cur, tar, 10, 5);
	TEST(Logic::isEqualLogic(inserted, result));
	delete[] inserted;
}

TEST_DEF(removeLogic_emptyCurrent) {
	// Test removeLogic function with an empty current string
	const char* cur = "";
	const char* result = "";
	char* removed = Logic::removeLogic(cur, 0, 0);
	TEST(Logic::isEqualLogic(removed, result));
	delete[] removed;
}

TEST_DEF(removeLogic_removeUntilEnd) {
	// Test removeLogic function by removing until the end of the current string
	const char* cur = "HelloWorld";
	const char* result = "World";
	char* removed = Logic::removeLogic(cur, 0, 5);
	TEST(Logic::isEqualLogic(removed, result));
	delete[] removed;
}

TEST_DEF(removeLogic_removeMiddleCharacters) {
	// Test removeLogic function by removing characters in the middle of the current string
	const char* cur = "HelloWorld";
	const char* result = "Helrld";
	char* removed = Logic::removeLogic(cur, 3, 7);
	TEST(Logic::isEqualLogic(removed, result));
	delete[] removed;
}

TEST_DEF(insertLogicGood) {
	// Test insertLogic function
	const char* cur = "Hello";
	const char* tar = "World";
	const char* result = "HelloWorld";
	char* inserted = Logic::insertLogic(cur, tar, 5, 5);
	TEST(Logic::isEqualLogic(inserted, result));
	delete[] inserted;
}

TEST_DEF(removeLogicGood) {
	// Test removeLogic function
	const char* cur = "HelloWorld";
	const char* result = "Helo";
	char* removed = Logic::removeLogic(cur, 3, 8);
	TEST(Logic::isEqualLogic(removed, result));
	delete[] removed;
}


TEST_DEF(insertLogic) {
	// Test insertLogic function
	testinsertLogicGood();
	testinsertLogic_emptyTarget();
	testinsertLogic_emptyCurrent();
	// testinsertLogic_curLengthLessThanPos();
}

TEST_DEF(removeLogic) {
	// Test removeLogic function
	// testremoveLogicGood();
	testremoveLogic_emptyCurrent();
	testremoveLogic_removeUntilEnd();
	testremoveLogic_removeMiddleCharacters();
}

// -------------------------------- Conversions --------------------------------------- //

TEST_DEF(convertStringToValue_alni) {
	const char* str1 = "123";
	const char* str2 = "456xyz";
	const char* str3 = "789";
	alni output;
	TEST(Logic::convertStringToValue(str1, output) && output == 123);
	TEST(!Logic::convertStringToValue(str2, output));
	TEST(Logic::convertStringToValue(str3, output) && output == 789);
}

TEST_DEF(convertStringToValue_alnf) {
	const char* str1 = "12.34";
	const char* str2 = "56.78xyz";
	const char* str3 = "90.12";
	alnf output;
	TEST(Logic::convertStringToValue(str1, output) && output == 12.34);
	TEST(!Logic::convertStringToValue(str2, output));
	TEST(Logic::convertStringToValue(str3, output) && output == 90.12);
}

TEST_DEF(convertStringToValue_bool) {
	const char* str1 = "true";
	const char* str2 = "false";
	const char* str3 = "1";
	const char* str4 = "0";
	bool output;
	TEST(Logic::convertStringToValue(str1, output) && output == true);
	TEST(Logic::convertStringToValue(str2, output) && output == false);
	TEST(Logic::convertStringToValue(str3, output) && output == true);
	TEST(Logic::convertStringToValue(str4, output) && output == false);
}

TEST_DEF(convertValueToString_alni) {
	alni input1 = 123;
	alni input2 = -456;
	char output[10];
	TEST(Logic::convertValueToString(input1, output, 10));
	TEST(Logic::isEqualLogic(output, "123"));

	TEST(Logic::convertValueToString(input2, output, 10));
	TEST(Logic::isEqualLogic(output, "-456"));
}

TEST_DEF(convertValueToString_alnf) {
	alnf input1 = 12.34;
	alnf input2 = -56.78;
	char output[10];

	TEST(Logic::convertValueToString(input1, output, 10));
	TEST(Logic::isEqualLogic(output, "12.340000"));

	TEST(Logic::convertValueToString(input2, output, 10));
	TEST(Logic::isEqualLogic(output, "-56.78000"));
}

TEST_DEF(convertValueToString_bool) {
	bool input1 = true;
	bool input2 = false;
	char output[10];

	TEST(Logic::convertValueToString(input1, output, 10));
	TEST(Logic::isEqualLogic(output, "true"));

	TEST(Logic::convertValueToString(input2, output, 10));
	TEST(Logic::isEqualLogic(output, "false"));
}

TEST_DEF(Conversions) {
	testconvertValueToString_bool();
	testconvertValueToString_alnf();
	testconvertValueToString_alni();
	testconvertStringToValue_bool();
	testconvertStringToValue_alnf();
	testconvertStringToValue_alni();
}

// ------------------------------------------------------------------------ //

TEST_DEF(isEqualLogic) {
	// Test isEqualLogic function
	const char* str1 = "Hello";
	const char* str2 = "Hello";
	const char* str3 = "World";
	const char* str4 = "Hello2";
	TEST(Logic::isEqualLogic(str1, str2));
	TEST(!Logic::isEqualLogic(str1, str4));
	TEST(!Logic::isEqualLogic(str1, str3));
}

TEST_DEF(calcLineOffsets) {
	// Test calcLineOffsets function
	const char* str = "\n\nHello\nWorld\n\n ";
	Buffer<Logic::Index> offsets;
	Logic::calcLineOffsets(str, Logic::calcLength(str), offsets);

	TEST(offsets.size() == 7);

	TEST(offsets[0] == 0);
	TEST(offsets[1] == 1);
	TEST(offsets[2] == 2);
	TEST(offsets[3] == 8);
	TEST(offsets[4] == 14);
	TEST(offsets[5] == 15);
	TEST(offsets[6] == 16);
}


TEST_DEF(StringLogic) {
	testisNewLineChar();
	testisEndChar();
	testcalcLength();

	testisEqualLogic();
	testcalcLineOffsets();

	testinsertLogic();
	testremoveLogic();

	testConversions();
}