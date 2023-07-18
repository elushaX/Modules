
#include "Testing.hpp"
#include "LocalStorage.hpp"

using namespace tp;

TEST_DEF_STATIC(Simple) {

	int1 data[] = { 0, 1, 2, 3, 4 };
	int1 dataRead[]{};

	{
		File file;
		file.connect(FileLocation(String("tmp2")), FileConnectionType(false));
		file.writeBytes(data, 5);
		file.disconnect();
	}

	{
		File file;
		file.connect(FileLocation(String("tmp2")), FileConnectionType(true));
		file.readBytes(dataRead, 5);
		file.disconnect();
	}

	for (auto i = 0; i < 5; i++) {
		TEST(data[i] == dataRead[i]);
	}
}

TEST_DEF(LocalStorage) {
	testSimple();
}