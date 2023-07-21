
#include "Testing.hpp"
#include "LocalStorage.hpp"

using namespace tp;

TEST_DEF_STATIC(Simple) {

	const int1* data = "abcde\0";
	int1 dataRead[6]{};

	{
		File file;
		file.connect(FileLocation(String("tmp2.txt")), FileConnectionType(false));
		file.writeBytes(data, 6);
		file.disconnect();
	}

	{
		File file;
		file.connect(FileLocation(String("tmp2.txt")), FileConnectionType(true));
		file.readBytes(dataRead, 6);
		file.disconnect();
	}

	for (auto i = 0; i < 5; i++) {
		TEST(data[i] == dataRead[i]);
	}
}

TEST_DEF(LocalStorage) {
	testSimple();
}