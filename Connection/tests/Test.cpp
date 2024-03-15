
#include "ConnectionCommon.hpp"

#include "LocalConnection.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;

SUITE(Connection) {
	TEST(LocalConnection) {
		const int1* data = "abcde\0";
		int1 dataRead[6]{};

		{
			LocalConnection file;
			file.connect(LocalConnection::Location(String("tmp2.txt")), LocalConnection::Type(false));
			file.writeBytes(data, 6);
			file.disconnect();
		}

		{
			LocalConnection file;
			file.connect(LocalConnection::Location(String("tmp2.txt")), LocalConnection::Type(true));
			file.readBytes(dataRead, 6);
			file.disconnect();
		}

		for (auto i = 0; i < 5; i++) {
			CHECK(data[i] == dataRead[i]);
		}
	}

	TEST(NO_TESTS) { CHECK(false); }
}

int main() {

	tp::ModuleManifest* deps[] = { &tp::gModuleConnection, nullptr };
	tp::ModuleManifest testModule("ConnectionTest", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	bool res = UnitTest::RunAllTests();

	testModule.deinitialize();

	return res;
}