
#include "RemoteConnection.hpp"
#include "Testing.hpp"

using namespace tp;

TEST_DEF_STATIC(Simple) {
	TEST(false);
}

TEST_DEF(RemoteConnection) { testSimple(); }
