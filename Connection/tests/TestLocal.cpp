
#include "LocalConnection.hpp"
#include "Testing.hpp"

using namespace tp;

TEST_DEF_STATIC(Simple) {

	const int1* data = "abcde\0";
	int1 dataRead[6]{};

	{
		LocalConnection file;
		file.connect(LocalConnectionLocation(String("tmp2.txt")), LocalConnectionType(false));
		file.writeBytes(data, 6);
		file.disconnect();
	}

	{
		LocalConnection file;
		file.connect(LocalConnectionLocation(String("tmp2.txt")), LocalConnectionType(true));
		file.readBytes(dataRead, 6);
		file.disconnect();
	}

	for (auto i = 0; i < 5; i++) {
		TEST(data[i] == dataRead[i]);
	}
}

TEST_DEF(LocalConnection) {
	testSimple();
}